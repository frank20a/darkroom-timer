#pragma once


class PID {
    public:
        PID(double kp, double ki, double kd) : kp(kp), ki(ki), kd(kd) {}

        double compute(double setpoint, double measured_value) {
            double error = setpoint - measured_value;
            integral += error;
            double derivative = error - prev_error;
            prev_error = error;
            return kp * error + ki * integral + kd * derivative;
        }

    private:
        double kp, ki, kd;
        double prev_error = 0;
        double integral = 0;
};

class Ramp {
    public:
        Ramp(float factor, float initial_value = 0.0f) : factor(factor), value(initial_value) {}
        float update(float setpoint) {
            value += factor * (setpoint - value);
            return value;
        }
        float get() { return value; }
        void set(float new_value) { value = new_value; }

    private:
        float factor, value;
};

class RampLinear {
    public:
        RampLinear(float step, float initial_value = 0.0f) : step(step), value(initial_value) {}
        float update(float setpoint) {
            if (value < setpoint)
                value = min(value + step, setpoint);
            else if (value > setpoint)
                value = max(value - step, setpoint);
            return value;
        }
        float get() { return value; }
        void set(float new_value) { value = new_value; }

    private:
        float step, value;
};
