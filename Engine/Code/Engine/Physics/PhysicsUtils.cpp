#include "Engine/Physics/PhysicsUtils.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/Collider.hpp"

Vector2 MathUtils::CalcClosestPoint(const Vector2& p, const Collider& collider) {
    return collider.Support((p - collider.CalcCenter()).GetNormalize());
}

static constexpr int maxGJKIterations = 25;

bool PhysicsUtils::GJKIntersect(const Collider& a, const Collider& b) {
    return GJK(a, b).collides;
}

//TODO: Multi-collision causes large simplex.
GJKResult PhysicsUtils::GJK(const Collider& a, const Collider& b) {
    using Simplex = std::vector<Vector3>;
    const auto calcMinkowskiDiff = [](const Vector2& direction, const Collider& a) { return a.Support(direction); };
    const auto support = [&](const Vector2& direction) { return calcMinkowskiDiff(direction, a) - calcMinkowskiDiff(-direction, b); };
    const auto initialDisplacement = b.CalcCenter() - a.CalcCenter();
    const auto initialDirection = initialDisplacement.GetNormalize();
    auto A = support(Vector2::X_Axis);
    Simplex simplex{Vector3{A}};
    auto D = Vector3{-A};
    const auto doSimplexLine = [&](Simplex& simplex, Vector3& D) {
        const auto& pointA = simplex[0];
        const auto& pointB = simplex[1];
        const auto lineAB = pointB - pointA;
        const auto lineAO = -pointA;
        if(MathUtils::DotProduct(lineAB, lineAO) > 0.0f) {
            D = MathUtils::TripleProductVector(lineAB, lineAO, lineAB);
        } else {
            D = lineAO;
            simplex = Simplex{Vector3{A}};
        }
        return false;
    };
    const auto doSimplexTriangle = [&](Simplex& simplex, Vector3& D) {
        const auto& pointA = simplex[0];
        const auto& pointB = simplex[1];
        const auto& pointC = simplex[2];
        const auto lineAB = pointB - pointA;
        const auto lineAC = pointC - pointA;
        const auto lineAO = (-pointA);
        const auto abc = MathUtils::CrossProduct(lineAB, lineAC);

        if(MathUtils::DotProduct(MathUtils::CrossProduct(abc, lineAC), lineAO) > 0.0f) {
            if(MathUtils::DotProduct(lineAC, lineAO) > 0.0f) {
                simplex = {pointA, pointC};
                D = MathUtils::TripleProductVector(lineAC, lineAO, lineAC);
            } else {
                simplex = {pointA, pointB};
                return doSimplexLine(simplex, D);
            }
        } else {
            if(MathUtils::DotProduct(MathUtils::CrossProduct(lineAB, abc), lineAO) > 0.0f) {
                simplex = {pointA, pointB};
                return doSimplexLine(simplex, D);
            } else {
                if(MathUtils::DotProduct(abc, lineAO) > 0.0f) {
                    D = abc;
                } else {
                    simplex = {pointA, pointC, pointB};
                    D = -abc;
                }
                return true;
            }
        }
        return false;
    };
    const auto doSimplex = [&](Simplex& simplex, Vector3& D) {
        const auto S = simplex.size();
        switch(S) {
        case 2:
            return doSimplexLine(simplex, D);
        case 3:
            return doSimplexTriangle(simplex, D);
        default:
            return false;
        }
    };
    const auto result = [&](Simplex& simplex, Vector3& D) {
        for(;;) {
            A = support(Vector2{D});
            if(MathUtils::DotProduct(A, Vector2{D}) <= 0.0f) {
                return false;
            }
            simplex.insert(std::begin(simplex), Vector3{A});
            const auto no_new_point = [&]() -> bool {
                if(simplex.size() < 2)
                    return false;
                const Vector3 previous1 = *simplex.rbegin();
                const Vector3 previous2 = *(simplex.rbegin() + 1);
                return MathUtils::IsEquivalent(previous1, previous2);
            }();
            if(no_new_point) {
                return false;
            }
            if(doSimplex(simplex, D)) {
                return true;
            }
        }
    }(simplex, D); //IIIL
    return GJKResult{result, simplex};
}

EPAResult PhysicsUtils::EPA(GJKResult gjk, const Collider& a, const Collider& b) {
    if(!gjk.collides) {
        return {};
    }

    const auto calcMinkowskiDiff = [](const Vector3& direction, const Collider& a) { return a.Support(Vector2{direction}); };
    const auto support = [&](const Vector3& direction) { return calcMinkowskiDiff(direction, a) - calcMinkowskiDiff(-direction, b); };

    std::size_t minIndex = 0u;
    constexpr auto infinity = std::numeric_limits<float>::infinity();
    auto minDistance = infinity;
    auto minNormal = Vector2::Zero;
    constexpr float epsilon = 0.0001f;

    while(minDistance == std::numeric_limits<float>::infinity()) {
        for(std::size_t i = 0u; i < gjk.simplex.size(); ++i) {
            std::size_t j = (i + 1u) % gjk.simplex.size();

            auto vertexI = Vector2(gjk.simplex[i]);
            auto vertexJ = Vector2(gjk.simplex[j]);

            auto ij = vertexJ - vertexI;
            auto normal = Vector2(ij.y, -ij.x).GetNormalize();
            auto distance = MathUtils::DotProduct(normal, vertexI);

            if(distance < 0.0f) {
                distance = -distance;
                normal = -normal;
            }
            if(distance < minDistance) {
                minDistance = distance;
                minNormal = normal;
                minIndex = j;
            }
        }

        const auto supportValue = support(Vector3{minNormal});
        const auto sDistance = MathUtils::DotProduct(minNormal, supportValue);

        if(std::abs(sDistance - minDistance) > epsilon) {
            minDistance = infinity;
            gjk.simplex.insert(std::begin(gjk.simplex) + minIndex, Vector3{supportValue});
        }
    }

    return {minDistance + epsilon, Vector3{minNormal}};
}

bool PhysicsUtils::SAT(const Collider& a, const Collider& b) {
    const auto* polyA = dynamic_cast<const ColliderPolygon*>(&a);
    const auto* polyB = dynamic_cast<const ColliderPolygon*>(&b);
    if(!(polyA && polyB)) {
        return false;
    }
    return MathUtils::DoPolygonsOverlap(polyA->GetPolygon(), polyB->GetPolygon());
}
