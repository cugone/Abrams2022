#pragma once

class World {
public:
    World() = default;
    World(const World& other) = default;
    World(World&& other) = default;
    World& operator=(const World& other) = default;
    World& operator=(World&& other) = default;
    ~World() = default;

protected:
private:
    
};
