#include "Planner.h"
#include <Arduino.h>
bool flag = false;
Planner::Planner(StepperController *stepper_c, struct segment_plan *seg_pl)
{
    stepper_c_ = stepper_c;
    segment_plan_ = seg_pl;
    current_drawing_ = nullptr;
    current_segment_ = 0;
    is_segment_printing_ = false;
    finished_drawing_ = true;
}

void Planner::print_stepper()
{
    const int *pos = stepper_c_->get_steps_count();
    Serial.println("POS:");
    Serial.print(pos[LEFT_STRIP_AXIS]);
    Serial.print(",");
    Serial.println(pos[RIGHT_STRIP_AXIS]);
}

void Planner::init_segment_plan(const int *target_pos)
{
    Serial.println("Loading segment.");
    segment_plan_->current_position = stepper_c_->get_steps_count();
    segment_plan_->target_position[LEFT_STRIP_AXIS] = target_pos[LEFT_STRIP_AXIS];
    segment_plan_->target_position[RIGHT_STRIP_AXIS] = target_pos[RIGHT_STRIP_AXIS];
    segment_plan_->led_pwm_value = target_pos[SERVO_ANGLE];
    segment_plan_->dx = abs(segment_plan_->current_position[LEFT_STRIP_AXIS] - target_pos[LEFT_STRIP_AXIS]);
    segment_plan_->dy = abs(segment_plan_->current_position[RIGHT_STRIP_AXIS] - target_pos[RIGHT_STRIP_AXIS]);
    segment_plan_->dominant_axis = (max(segment_plan_->dx, segment_plan_->dy));
    segment_plan_->x_step_value = 0;
    segment_plan_->y_step_value = 0;
    // Set steps directions
    segment_plan_->current_direction_mask = get_line_direction_mask(segment_plan_->current_position, segment_plan_->target_position);
    segment_plan_->current_step_mask = 0;
    // _segment_plan->segment_is_enabled = false;
}

void Planner::print_segment()
{
    Serial.print("current step mask: ");
    Serial.print(segment_plan_->current_step_mask);
    Serial.print(",");
    Serial.println(segment_plan_->current_direction_mask);
    Serial.print("current pos: ");
    Serial.print(segment_plan_->current_position[LEFT_STRIP_AXIS]);
    Serial.print(",");
    Serial.println(segment_plan_->current_position[RIGHT_STRIP_AXIS]);


    Serial.print("target pos: ");
    Serial.print(segment_plan_->target_position[LEFT_STRIP_AXIS]);
    Serial.print(",");
    Serial.print(segment_plan_->target_position[RIGHT_STRIP_AXIS]);
    Serial.print(",");
    Serial.print("deltas: ");
    Serial.print(segment_plan_->dx);
    Serial.print(",");
    Serial.print(segment_plan_->dy);
    Serial.print(",");
    Serial.print(",");
    Serial.println(segment_plan_->dominant_axis);
    Serial.print("step values : ");
    Serial.print(segment_plan_->x_step_value);
    Serial.print(",");
    Serial.println(segment_plan_->y_step_value);

    Serial.println("--------------------------");
}

void Planner::print_steps()
{
    Serial.println("steps values:");
    Serial.print(segment_plan_->x_step_value);
    Serial.print(",");
    Serial.println(segment_plan_->y_step_value);
    Serial.println("_________________________________");
}

void Planner::print_segment_positions()
{
    Serial.print("current pos: ");
    Serial.print(segment_plan_->current_position[LEFT_STRIP_AXIS]);
    Serial.print(",");
    Serial.println(segment_plan_->current_position[RIGHT_STRIP_AXIS]);
    Serial.print("target pos: ");
    Serial.print(segment_plan_->target_position[LEFT_STRIP_AXIS]);
    Serial.print(",");
    Serial.println(segment_plan_->target_position[RIGHT_STRIP_AXIS]);

    Serial.println("______________________________________");
}

void Planner::move_to_position()
{
    const int *current_position = stepper_c_->get_steps_count();

    if (current_position[LEFT_STRIP_AXIS] != segment_plan_->target_position[LEFT_STRIP_AXIS] ||
        current_position[RIGHT_STRIP_AXIS] != segment_plan_->target_position[RIGHT_STRIP_AXIS])
    {
        /** START PRINTING **/
        if (!is_segment_printing_)
        {
            is_segment_printing_ = true;
            stepper_c_->set_enable(true);
            stepper_c_->set_direction(segment_plan_->current_direction_mask);
            stepper_c_->set_servo_value(segment_plan_->led_pwm_value);
        }

        segment_plan_->current_step_mask = 0;
        // Extract X step value
        if (current_position[LEFT_STRIP_AXIS] != segment_plan_->target_position[LEFT_STRIP_AXIS])
        {
            if (segment_plan_->dx && (segment_plan_->x_step_value >= segment_plan_->dominant_axis))
            {
                segment_plan_->current_step_mask = segment_plan_->current_step_mask | (1 << LEFT_STRIP_AXIS);
                segment_plan_->x_step_value -= segment_plan_->dominant_axis;
            }
            segment_plan_->x_step_value += segment_plan_->dx;
        }
        // Extract Y step value
        if (current_position[RIGHT_STRIP_AXIS] != segment_plan_->target_position[RIGHT_STRIP_AXIS])
        {
            if (segment_plan_->dy && (segment_plan_->y_step_value >= segment_plan_->dominant_axis))
            {
                segment_plan_->current_step_mask = segment_plan_->current_step_mask | (1 << RIGHT_STRIP_AXIS);
                segment_plan_->y_step_value -= segment_plan_->dominant_axis;
            }
            segment_plan_->y_step_value += segment_plan_->dy;
        }


        stepper_c_->move_step(segment_plan_->current_step_mask, segment_plan_->current_direction_mask);
    }
    else
    {
        // Serial.println("got here");
        // stepper_c_->set_enable(false);
        is_segment_printing_ = false;
    }
    // print_stepper();
}

int Planner::get_line_direction_mask(const int *point1, const int *point2)
{
    const int x_direction_sign = sgn(point2[LEFT_STRIP_AXIS] - point1[LEFT_STRIP_AXIS]);
    const int y_direction_sign = sgn(point2[RIGHT_STRIP_AXIS] - point1[RIGHT_STRIP_AXIS]);
    int current_direction_mask = 0;
    if (x_direction_sign && x_direction_sign < 0)
    {
        current_direction_mask = current_direction_mask | (1 << LEFT_STRIP_AXIS);
    }
    if (y_direction_sign && y_direction_sign < 0)
    {
        current_direction_mask = current_direction_mask | (1 << RIGHT_STRIP_AXIS);
    }

    return current_direction_mask;
}

void Planner::load_drawing(Drawing *current_drawing)
{
    finished_drawing_ = false;
    current_drawing_ = current_drawing;
    stepper_c_->set_steps_rate(current_drawing->drawing_speed_);
    Serial.println("Start ploting drawing");
    Serial.print("first segment:");
    Serial.print(pgm_read_word(&current_drawing_->segments_[current_segment_][LEFT_STRIP_AXIS]));
    Serial.print(",");
    Serial.println(pgm_read_word(&current_drawing_->segments_[current_segment_][RIGHT_STRIP_AXIS]));
    random_x_val = 0;
    random_y_val = 0;
    if(current_drawing->is_random_){
      random_x_val = int16_t(random(-10,10)*20);
      random_y_val = int16_t(random(-10,10)*20);
      Serial.print("random offset:");
      Serial.print(random_x_val);
      Serial.print(",");
      Serial.println(random_y_val);
    }    
}

void Planner::plot_drawing()
{
    if (!finished_drawing_)
    {
        // Serial.println(current_drawing_size_);
        if (!is_segment_printing_ && current_segment_ < current_drawing_->drawing_size_)
        {
            // Converting current target position into steps for the polar coordinate system
            long L1 = 0, L2 = 0;
            int16_t x_val = (int16_t)pgm_read_word(&current_drawing_->segments_[current_segment_][LEFT_STRIP_AXIS]) + random_x_val;
            int16_t y_val = (int16_t)pgm_read_word(&current_drawing_->segments_[current_segment_][RIGHT_STRIP_AXIS]) + random_y_val;
            convert_to_polar(x_val,y_val,&L1,&L2);

            const int target_to_steps[N_INSTRUCTIONS] = {
                int(L1),
                int(L2),
                pgm_read_word(&current_drawing_->segments_[current_segment_][SERVO_ANGLE])};

            // start of segment
            init_segment_plan(target_to_steps);
            print_segment_positions();
            // print_segment();
            // print_stepper();
            move_to_position();
            Serial.print("Finished segment: ");
            Serial.println(current_segment_);

            current_segment_++;
        }
        else if (is_segment_printing_)
        {
            move_to_position();
        }
        else if (current_segment_ == current_drawing_->drawing_size_)
        {
            reset_drawing();
        }
    }
}

bool Planner::is_drawing_finished()
{
    return finished_drawing_;
}

void Planner::reset_drawing()
{
    print_stepper();
    Serial.println("Finished plotting drawing");
    // Serial.println(current_drawing_->drawing_name_);
    current_drawing_ = nullptr;
    current_segment_ = 0;
    finished_drawing_ = true;
    is_segment_printing_ = false;
    // stepper_c_->set_servo_value(0);
}

void Planner::test_print()
{
    Serial.print("testing: ");
    Serial.println(current_drawing_->drawing_size_);
    for (int i = 0; i < current_drawing_->drawing_size_; i++)
    {
        Serial.print(pgm_read_word(&(current_drawing_->segments_)[i][LEFT_STRIP_AXIS]));
        Serial.print(",");
        Serial.println(pgm_read_word(&(current_drawing_->segments_)[i][RIGHT_STRIP_AXIS]));

    }
}

void Planner::convert_to_cartesian(int &x, int &y){
  // Extract the current length of the timing strips in MM
  double l1 = (stepper_c_->get_steps_count()[LEFT_STRIP_AXIS]/LEFT_STEPS_PER_MM);
  double l2 = (stepper_c_->get_steps_count()[RIGHT_STRIP_AXIS]/RIGHT_STEPS_PER_MM);

  // calculate the current height and width
  /**
  h = ?, w = ?
  1. (dis - w)^2 + h^2 = l1^2
  2. w^2 + h^2 = l2^2
   --> (motors_distance-w)^2 - w^2 = l1^2 - l2^2
   --> motors_distance^2 -2*motors_distance*w  = l1^2 - l2^2
  **/
  
  double w = (l1*l1 - l2*l2 - double(MOTORS_DISTANCE)*MOTORS_DISTANCE)/ (-2*double(MOTORS_DISTANCE));
  double h = sqrt(l2*l2 - w*w);
  x = int(double(MOTORS_DISTANCE) - w + X_OFFSET);
  y = int(-1*(h + Y_OFFSET));
}

void Planner::convert_to_polar(int16_t x,int16_t y,long *l1, long *l2) {
  long h = (-1*int(y)) - long(Y_OFFSET);
  long w = -1*(int(x) - long(X_OFFSET) - long(MOTORS_DISTANCE));
  *l2 = long(sqrt(h*h+w*w)*LEFT_STEPS_PER_MM);
  w = w - int(MOTORS_DISTANCE);
  *l1 = long(sqrt(h*h+w*w)*RIGHT_STEPS_PER_MM);
}
