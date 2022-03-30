#include "Engine/Physics/Particles/Particle.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Renderer/Material.hpp"

ParticleIntegrator Particle::semi_implicit_euler = [](float /*time*/, const ParticleState& state) -> ParticleState {
    ParticleState dS = state;
    if(dS.mass > 0.0f) {
        dS.acceleration = Vector3::Zero;
        dS.velocity += state.acceleration;
        dS.position += state.velocity;
    }
    return dS;
};

Particle::Particle(const ParticleRenderState& initialRenderState, const ParticleState& initialState)
: m_renderState(initialRenderState)
, m_nextState(initialState)
, m_curState(initialState)
, m_model(semi_implicit_euler)
{
    /* DO NOTHING */
}

Particle::Particle(const ParticleRenderState& initialRenderState, const ParticleState& initialState, const ParticleIntegrator& model)
: m_renderState(initialRenderState)
, m_nextState(initialState)
, m_curState(initialState)
, m_model(model)
{
    /* DO NOTHING */
}

const ParticleState& Particle::GetState() const {
    return m_nextState;
}

ParticleState& Particle::GetState() {
    return m_nextState;
}

const ParticleRenderState& Particle::GetRenderState() const {
    return m_renderState;
}

ParticleRenderState& Particle::GetRenderState() {
    return m_renderState;
}

void Particle::SetIntegrationModel(const ParticleIntegrator& model) {
    m_model = model;
}

void Particle::Update(float time, float deltaSeconds) {
    m_renderState.m_age -= deltaSeconds;

    if(m_renderState.m_age <= 0.0f) {
        Kill();
        return;
    }

    //Update visuals
    float ageRatio = m_renderState.m_age / m_renderState.m_start_age;
    //Interpolate "backwards" because age starts at 1 and goes to 0
    m_renderState.m_scale = MathUtils::Interpolate(m_renderState.m_end_scale, m_renderState.m_start_scale, ageRatio);
    m_renderState.m_color = MathUtils::Interpolate(m_renderState.m_end_color, m_renderState.m_start_color, ageRatio);

    //Update physics
    m_nextState = m_curState + m_model(time, m_nextState) * deltaSeconds;
    m_curState = m_nextState;
}

void Particle::Render(Mesh::Builder& builder) const {
    if(IsTransparent()) {
        return;
    }

    const auto vert_left = m_curState.position.x - 0.5f;
    const auto vert_right = m_curState.position.x + 0.5f;
    const auto vert_top = m_curState.position.y - 0.5f;
    const auto vert_bottom = m_curState.position.y + 0.5f;

    const auto vert_bl = Vector2(vert_left, vert_bottom);
    const auto vert_tl = Vector2(vert_left, vert_top);
    const auto vert_tr = Vector2(vert_right, vert_top);
    const auto vert_br = Vector2(vert_right, vert_bottom);

    const auto tx_left = 0.0f;
    const auto tx_right = 1.0f;
    const auto tx_top = 0.0f;
    const auto tx_bottom = 1.0f;

    const auto tx_bl = Vector2(tx_left, tx_bottom);
    const auto tx_tl = Vector2(tx_left, tx_top);
    const auto tx_tr = Vector2(tx_right, tx_top);
    const auto tx_br = Vector2(tx_right, tx_bottom);

    builder.Begin(PrimitiveType::Triangles);
    builder.SetColor(m_renderState.m_color);

    switch(m_renderState.m_shape) {
    case ParticleRenderState::ParticleShape::Quad: {
        builder.SetUV(tx_bl);
        builder.SetNormal(Vector3::Z_Axis);
        builder.AddVertex(vert_bl);

        builder.SetUV(tx_tl);
        builder.AddVertex(vert_tl);

        builder.SetUV(tx_tr);
        builder.AddVertex(vert_tr);

        builder.SetUV(tx_br);
        builder.AddVertex(vert_br);

        builder.AddIndicies(Mesh::Builder::Primitive::Quad);
        break;
    }
    case ParticleRenderState::ParticleShape::Cube: {
        const auto left = Vector3{-0.5f, 0.0f, 0.0f};
        const auto right = Vector3{0.5f, 0.0f, 0.0f};
        const auto up = Vector3{0.0f, 0.5f, 0.0f};
        const auto down = Vector3{0.0f, -0.5f, 0.0f};
        const auto forward = Vector3{0.0f, 0.0f, -0.5f};
        const auto back = Vector3{0.0f, 0.0f, 0.5f};

        const Vector3 v_ldf = left + down + forward;
        const Vector3 v_ldb = left + down + back;
        const Vector3 v_luf = left + up + forward;
        const Vector3 v_lub = left + up + back;
        const Vector3 v_ruf = right + up + forward;
        const Vector3 v_rub = right + up + back;
        const Vector3 v_rdf = right + down + forward;
        const Vector3 v_rdb = right + down + back;

        //Front
        builder.SetNormal(-Vector3::Z_Axis);
        builder.AddVertex(v_rdf);
        builder.AddVertex(v_ldf);
        builder.AddVertex(v_luf);
        builder.AddVertex(v_ruf);
        builder.AddIndicies(Mesh::Builder::Primitive::Quad);

        //Back
        builder.SetNormal(Vector3::Z_Axis);
        builder.AddVertex(v_ldb);
        builder.AddVertex(v_rdb);
        builder.AddVertex(v_rub);
        builder.AddVertex(v_lub);
        builder.AddIndicies(Mesh::Builder::Primitive::Quad);

        //Left
        builder.SetNormal(-Vector3::X_Axis);
        builder.AddVertex(v_ldf);
        builder.AddVertex(v_ldb);
        builder.AddVertex(v_lub);
        builder.AddVertex(v_luf);
        builder.AddIndicies(Mesh::Builder::Primitive::Quad);

        //Right
        builder.SetNormal(Vector3::X_Axis);
        builder.AddVertex(v_rdb);
        builder.AddVertex(v_rdf);
        builder.AddVertex(v_ruf);
        builder.AddVertex(v_rub);
        builder.AddIndicies(Mesh::Builder::Primitive::Quad);

        //Top
        builder.SetNormal(Vector3::Y_Axis);
        builder.AddVertex(v_ruf);
        builder.AddVertex(v_luf);
        builder.AddVertex(v_lub);
        builder.AddVertex(v_rub);
        builder.AddIndicies(Mesh::Builder::Primitive::Quad);

        //Bottom
        builder.SetNormal(-Vector3::Y_Axis);
        builder.AddVertex(v_rdb);
        builder.AddVertex(v_ldb);
        builder.AddVertex(v_ldf);
        builder.AddVertex(v_rdf);
        builder.AddIndicies(Mesh::Builder::Primitive::Quad);
        break;
    }
    }
    builder.End(m_renderState.m_particle_material);
}

bool Particle::IsTransparent() const {
    return m_renderState.m_color.a == 0;
}

bool Particle::IsAlive() const {
    return m_renderState.m_age > 0.0f;
}

bool Particle::IsDead() const {
    return !IsAlive();
}
void Particle::Kill() {
    m_renderState.m_age = 0.0f;
}

const Matrix4& Particle::GetParentTransform() const {
    return m_parentTransform;
}

Matrix4& Particle::GetParentTransform() {
    return m_parentTransform;
}

void Particle::SetParentTransform(const Matrix4& transform) {
    m_parentTransform = transform;
}

bool operator<(const Particle& a, const Particle& b) {
    return a.GetRenderState().GetColor().a < b.GetRenderState().GetColor().a;
}

void ParticleRenderState::SetLifetime(float lifetimeSeconds) {
    m_start_age = lifetimeSeconds;
    m_age = m_start_age;
}

void ParticleRenderState::SetScales(const Vector3& start, const Vector3& end) {
    m_start_scale = start;
    m_end_scale = end;
    m_scale = start;
}

void ParticleRenderState::SetColors(const Rgba& start, const Rgba& end) {
    m_start_color = start;
    m_end_color = end;
    m_color = start;
}

const Rgba& ParticleRenderState::GetColor() const {
    return m_color;
}

Rgba& ParticleRenderState::GetColor() {
    return m_color;
}

void ParticleRenderState::SetMaterial(Material* material) {
    m_particle_material = material;
}

void ParticleRenderState::SetBillboarding(bool isBillboarded) {
    m_billboarded = isBillboarded;
}

void ParticleRenderState::SetShape(const ParticleShape& renderShape) {
    m_shape = renderShape;
}
const ParticleRenderState::ParticleShape& ParticleRenderState::GetShape() const {
    return m_shape;
}

const Rgba& ParticleRenderState::GetStartColor() const {
    return m_start_color;
}

const Rgba& ParticleRenderState::GetEndColor() const {
    return m_end_color;
}

const Vector3& ParticleRenderState::GetStartScale() const {
    return m_start_scale;
}

const Vector3& ParticleRenderState::GetEndScale() const {
    return m_end_scale;
}

//------------------------------------------------------------------------------
ParticleState::ParticleState()
: position(0.0f, 0.0f, 0.0f)
, velocity(0.0f, 0.0f, 0.0f)
, acceleration(0.0f, 0.0f, 0.0f)
, mass(1.0f) {
    /* DO NOTHING */
}

ParticleState ParticleState::operator*=(float scalar) {
    acceleration *= scalar;
    velocity *= scalar;
    position *= scalar;
    return *this;
}

ParticleState ParticleState::operator*(float scalar) {
    ParticleState ps = *this;
    ps.acceleration = acceleration * scalar;
    ps.velocity = velocity * scalar;
    ps.position = position * scalar;
    return ps;
}

ParticleState ParticleState::operator+=(const ParticleState& rhs) {
    velocity += rhs.acceleration;
    position += rhs.velocity;
    return *this;
}

ParticleState ParticleState::operator+(const ParticleState& rhs) {
    ParticleState ps = *this;
    ps.velocity += rhs.acceleration;
    ps.position += rhs.velocity;
    return ps;
}