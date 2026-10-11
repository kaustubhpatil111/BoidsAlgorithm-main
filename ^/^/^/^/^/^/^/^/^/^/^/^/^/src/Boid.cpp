#include "Boid.h"
#include <random>

Boid::Boid(
    const Vector2& position,
    const Vector2& velocity,
    const Vector2& acceleration
)
    : position(position),
      velocity(velocity),
      acceleration(acceleration),
      maxSpeed(10.0f),
      minimumSpeed(2.0f),
      slowdownRadius(5.0f),
      neighborRadius(5.0f),
      maxAcceleration(4.0f),
      wanderDirection(1.0f,0.0f)

{
}

void Boid::update(float deltaTime)
{

    velocity = velocity + acceleration * deltaTime;

    float speed = velocity.magnitude();
    
    if(speed >maxSpeed){
        velocity= velocity.normalized()*maxSpeed;
    }
    else if(speed <minimumSpeed && speed >0.0f){
        velocity=velocity.normalized()*minimumSpeed;
    }

    position = position + velocity * deltaTime;
}

void Boid::seek(const Vector2& target)
{
    Vector2 direction = target - position;

    float distance = direction.magnitude();

    float desiredSpeed = maxSpeed;

    if(distance < slowdownRadius){

        desiredSpeed = maxSpeed * (distance / slowdownRadius);
    }

    Vector2 desiredVelocity = direction.normalized() * desiredSpeed;

    acceleration = desiredVelocity - velocity;

}

Vector2 Boid::separationFrom(const Boid& other){
    Vector2 difference = position - other.position ;
    float distance = difference.magnitude();
    
    if(distance == 0.0f){
        return Vector2(0.0f, 0.0f);
    }

    return difference.normalized()*(1.0f/distance);
}

Vector2 Boid::separation(const std::vector<Boid> & boids){
    Vector2 total(0.0f , 0.0f);
    for (const Boid&other : boids){
        if (&other == this){
            continue;
        }
        float distance= position.distanceTo(other.position);

        if (distance <= neighborRadius){
            total=total+separationFrom(other);
        }
    }
    return total;

}

Vector2 Boid::alignment(const std::vector<Boid>& boids){
    Vector2 averageVelocity(0.0f,0.0f);
    int neighborCount=0;

    for (const Boid& other : boids){
        if(&other == this){
            continue;
        }
        float distance=position.distanceTo(other.position);
        if(distance<= neighborRadius){
            averageVelocity=averageVelocity +other.velocity;
            neighborCount++;
        }
    }

        if(neighborCount ==0){
            return Vector2(0.0f, 0.0f);
        }

        averageVelocity=averageVelocity/neighborCount;
        return averageVelocity-velocity;
    
}

Vector2 Boid::cohesion(const std::vector<Boid>& boids){
    Vector2 centerOfMass(0.0f,0.0f);
    int neighborCount=0;

    for (const Boid& other: boids){
        if(&other == this){
            continue;
        }
        float distance=position.distanceTo(other.position);
        if(distance<= neighborRadius){
            centerOfMass=centerOfMass+other.position;
            neighborCount++;

        }
    }

    if(neighborCount==0){
        return Vector2(0.0f,0.0f);
    }
    centerOfMass=centerOfMass/neighborCount;

    Vector2 direction=centerOfMass - position;

    float distance =direction.magnitude();

    if(distance ==0.0f){
        return Vector2(0.0f,0.0f);
    }

    float desiredSpeed= maxSpeed;
    if(distance<slowdownRadius){
        desiredSpeed=maxSpeed*(distance/slowdownRadius);
    }
    Vector2 desiredVelocity=direction.normalized()*desiredSpeed;

    return desiredVelocity-velocity;
}

Vector2 Boid::steeringFrom(const std::vector<Boid>&boids){
    Vector2 separationForce =separation(boids);
    Vector2 alignmentForce=alignment(boids);
    Vector2 cohesionForce= cohesion(boids);
    Vector2 wanderForce=wander();

    float separationWeight=2.0f;
    float alignmentWeight = 1.0f;
    float cohesionWeight=0.5f;
    float wanderWeight=0.2f;

    Vector2 totalSteering=separationForce*separationWeight+
    alignmentForce*alignmentWeight
    +cohesionForce*cohesionWeight+
    wanderForce*wanderWeight;

    float steeringMagnitude = totalSteering.magnitude();

    if(steeringMagnitude > maxAcceleration){
        totalSteering = totalSteering.normalized() * maxAcceleration;
    }

    return totalSteering;

}

Vector2 Boid::wander(){
    static std::random_device rd;
    static std::mt19937 generator(rd());

    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    

    wanderDirection=wanderDirection+Vector2(
        distribution(generator),
        distribution(generator)
    );

    return wanderDirection.normalized();
}

void Boid::wrapAround(
    float minX,
    float maxX,
    float minY,
    float maxY
){
    if(position.x<minX){
        position.x=maxX;
    }
    else if(position.x>maxX){
        position.x=minX;
    }
    if(position.y<minY){
        position.y =maxY;
    }
    else if(position.y>maxY){
        position.y =minY;
    }
}