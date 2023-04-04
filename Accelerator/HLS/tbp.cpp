#include "tbp.hpp"
        
float squared(float x){
    return x*x;
}

float cubed(float x){
    return x*x*x;
}

float distance(body& body_0, body& body_1){
    float x = squared(body_0.position[0] - body_1.position[0]);
    float y = squared(body_0.position[1] - body_1.position[1]);
    return (float)sqrt(x + y);
}

float acceleration(float mass_0, body& body_0, body& body_1){
    float distance_ = distance(body_0, body_1);
    float denominator = squared(distance_);
    if(denominator < 0.01){
        return 0.01;
    }else{
        return G * mass_0 / denominator;
    }
}

void direction(body& body_0, body& body_1, float out_direction[2]){
    float distance_ = distance(body_0, body_1);
    if(distance_ < 0.01){
        distance_ = 0.01;
    }
    float x = (body_0.position[0] - body_1.position[0]) / distance_;
    float y = (body_0.position[1] - body_1.position[1]) / distance_;
    out_direction[0] = x;
    out_direction[1] = y;
}

void acceleration_step(body& body_0, body system[NUM_BODIES], float out_acceleration[2]){
    float out_direction[2] = {0.0, 0.0};
    for(int i = 0; i < NUM_BODIES; i++){
        body body_ = system[i];
        if(body_.id != body_0.id){
            float accel = acceleration(body_.mass, body_0, body_);
            direction(body_0, body_, out_direction);
            out_acceleration[0] += out_direction[0] * accel;
            out_acceleration[1] += out_direction[1] * accel;
        }
    }
}

void velocity_step(float inital_velocity[2], float time_step, float acceleration[2], float out_velocity[2]){
    float out_velocity_x = inital_velocity[0] + acceleration[0] * time_step;
    float out_velocity_y = inital_velocity[1] + acceleration[1] * time_step;
    out_velocity[0] = out_velocity_x;
    out_velocity[1] = out_velocity_y;
}

void postion_step(float inital_position[2], float initial_velocity[2], float time_step, float acceleration[2], float out_position[2]){
    float out_position_x = inital_position[0] + initial_velocity[0] * time_step + 0.5 * acceleration[0] * squared(time_step);
    float out_position_y = inital_position[1] + initial_velocity[1] * time_step + 0.5 * acceleration[1] * squared(time_step);
    out_position[0] = out_position_x;
    out_position[1] = out_position_y;
}

void step(body& body_0, float initial_position[2], float initial_velocity[2], float initial_acceleration[2], body system[NUM_BODIES], float time_step){
    float out_acceleration[2];
    float out_velocity[2];
    float out_position[2];
    acceleration_step(body_0, system, out_acceleration);
    velocity_step(initial_velocity, time_step, initial_acceleration, out_velocity);
    postion_step(initial_velocity, initial_position, time_step, initial_acceleration, out_position);
    body_0.acceleration[0] = out_acceleration[0];
    body_0.acceleration[1] = out_acceleration[1];
    body_0.velocity[0] = out_velocity[0];
    body_0.velocity[1] = out_velocity[1];
    body_0.position[0] = out_position[0];
    body_0.position[1] = out_position[1];
}

void total_step(body system[NUM_BODIES], float time_step){
    for(int i = 0; i < NUM_BODIES; i++){
        body body_0 = system[i];
        step(body_0, body_0.velocity, body_0.position, body_0.acceleration, system, time_step);
    }
}

template <typename T> void tbp(body system[NUM_BODIES], float time_step){
    total_step(system, time_step);
}

void three_body_problem(float time_step, stream &signal, stream &result) {
#pragma HLS INTERFACE s_axilite port=time_step
#pragma HLS INTERFACE axis port=signal
#pragma HLS INTERFACE axis port=result
#pragma HLS INTERFACE ap_ctrl_none port=return

    DataType in[N];
    DataType out[N];

read:

    for(int i = 0; i < N; i++){
        packet temp = signal.read();
        in[i] = temp.data;
    }

    body system[NUM_BODIES];
    for(int i = 0; i < N; i++){
        if(i % 8 == 0){
            int j = (int)(i / 8);
            body new_body;
            new_body.id = in[j];
            new_body.mass = in[j + 1];
            new_body.position[0] = in[j + 2];
            new_body.position[1] = in[j + 3];
            new_body.velocity[0] = in[j + 4];
            new_body.velocity[1] = in[j + 5];
            new_body.acceleration[0] = in[j + 6];
            new_body.acceleration[1] = in[j + 7];
            system[j] = new_body;
        }
    }

    tbp<DataType>(system, time_step);
    for(int j = 0; j < NUM_BODIES; j++){
        int i = 8 * j;
        out[i] = system[j].id;
        out[i + 1] = system[j].mass;
        out[i + 2] = system[j].position[0];
        out[i + 3] = system[j].position[1];
        out[i + 4] = system[j].velocity[0];
        out[i + 5] = system[j].velocity[1];
        out[i + 6] = system[j].acceleration[0];
        out[i + 7] = system[j].acceleration[1];
    }

write:
    for(int i = 0; i < N; i++){
        packet temp;
        temp.data = out[i];
        ap_uint<1> last = 0;
        if(i == N - 1){
            last = 1;
        }
        temp.last = last;
        temp.keep = -1;
        result.write(temp);
    }
}
