#include "Engine/Physics/Particles/Particle.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Renderer/Material.hpp"

ParticleIntegrator Particle::semi_implicit_euler = [=](float /*time*/, const ParticleState& state) -> ParticleState {
    ParticleState dS = state;
    if(dS.mass > 0.0f) {
        dS.acceleration = Vector3::Zero;
        dS.velocity += state.acceleration;
        dS.position += state.velocity;
    }
    return dS;
};

Particle::Particle(const ParticleRenderState& initialRenderState, const ParticleState& initialState)
: _renderState(initialRenderState)
, _nextState(initialState)
, _curState(initialState)
, _model(semi_implicit_euler)
{
    /* DO NOTHING */
}

Particle::Particle(const ParticleRenderState& initialRenderState, const ParticleState& initialState, const ParticleIntegrator& model)
: _renderState(initialRenderState)
, _nextState(initialState)
, _curState(initialState)
, _model(model)
{
    /* DO NOTHING */
}

const ParticleState& Particle::GetState() const {
    return _nextState;
}

ParticleState& Particle::GetState() {
    return _nextState;
}

const ParticleRenderState& Particle::GetRenderState() const {
    return _renderState;
}

ParticleRenderState& Particle::GetRenderState() {
    return _renderState;
}

void Particle::SetIntegrationModel(const ParticleIntegrator& model) {
    _model = model;
}

void Particle::Update(float time, float deltaSeconds) {
    _renderState.age -= deltaSeconds;

    if(_renderState.age <= 0.0f) {
        Kill();
        return;
    }

    //Update visuals
    float ageRatio = _renderState.age / _renderState.start_age;
    //Interpolate "backwards" because age starts at 1 and goes to 0
    _renderState.scale = MathUtils::Interpolate(_renderState.end_scale, _renderState.start_scale, ageRatio);
    _renderState.color = MathUtils::Interpolate(_renderState.end_color, _renderState.start_color, ageRatio);

    //Update physics
    _nextState = _curState + _model(time, _nextState) * deltaSeconds;
    _curState = _nextState;
}

void Particle::Render(Mesh::Builder& builder) const {
    if(IsTransparent()) {
        return;
    }

    const auto vert_left = _curState.position.x - 0.5f;
    const auto vert_right = _curState.position.x + 0.5f;
    const auto vert_top = _curState.position.y - 0.5f;
    const auto vert_bottom = _curState.position.y + 0.5f;

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
    builder.SetColor(_renderState.color);

    switch(_renderState.shape) {
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
    builder.End(_renderState.particle_material);
}

bool Particle::IsTransparent() const {
    return _renderState.color.a == 0;
}

bool Particle::IsAlive() const {
    return _renderState.age > 0.0f;
}

bool Particle::IsDead() const {
    return !IsAlive();
}
void Particle::Kill() {
    _renderState.age = 0.0f;
}

const Matrix4& Particle::GetParentTransform() const {
    return _parentTransform;
}

Matrix4& Particle::GetParentTransform() {
    return _parentTransform;
}

void Particle::SetParentTransform(const Matrix4& transform) {
    _parentTransform = transform;
}

bool operator<(const Particle& a, const Particle& b) {
    return a.GetRenderState().GetColor().a < b.GetRenderState().GetColor().a;
}

void ParticleRenderState::SetLifetime(float lifetimeSeconds) {
    start_age = lifetimeSeconds;
    age = start_age;
}

void ParticleRenderState::SetScales(const Vector3& start, const Vector3& end) {
    start_scale = start;
    end_scale = end;
    scale = start;
}

void ParticleRenderState::SetColors(const Rgba& start, const Rgba& end) {
    start_color = start;
    end_color = end;
    color = start;
}

const Rgba& ParticleRenderState::GetColor() const {
    return color;
}

Rgba& ParticleRenderState::GetColor() {
    return color;
}

void ParticleRenderState::SetMaterial(Material* material) {
    particle_material = material;
}

void ParticleRenderState::SetBillboarding(bool isBillboarded) {
    billboarded = isBillboarded;
}

void ParticleRenderState::SetShape(const ParticleShape& renderShape) {
    shape = renderShape;
}
const ParticleRenderState::ParticleShape& ParticleRenderState::GetShape() const {
    return shape;
}

const Rgba& ParticleRenderState::GetStartColor() const {
    return start_color;
}

const Rgba& ParticleRenderState::GetEndColor() const {
    return end_color;
}

const Vector3& ParticleRenderState::GetStartScale() const {
    return start_scale;
}

const Vector3& ParticleRenderState::GetEndScale() const {
    return end_scale;
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