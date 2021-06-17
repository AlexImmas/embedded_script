#pragma once

// @file AC_NonLinearControl.h
// Ardusub nonlinear control library

#include <Eigen/Dense>
using namespace Eigen;

#include <AP_Common/AP_Common.h>               
#include <AP_Param/AP_Param.h>
#include <AP_Vehicle/AP_Vehicle.h>              // common vehicle parameters
#include <AP_AHRS/AP_AHRS_View.h>
#include <AP_Motors/AP_Motors.h>                // motors library
#include <AP_Motors/AP_MotorsMulticopter.h>
#include <AP_InertialNav/AP_InertialNav.h>      // Inertial Navigation library
#include <AC_AFLC/AC_AFLC_4D.h>


// Controller parameters (used in constructor)
#define N                     4                // Model dof
#define BETA1                 1.0f             // Reference model bandwidth in surge
#define BETA2                 2.0f             // Reference model bandwidth in sway
#define BETA3                 3.0f             // Reference model bandwidth in heave
#define BETA4                 4.0f             // Reference model bandwidth in yaw
#define LAMBDA1               0.4f             // Pole placement gain in surge
#define LAMBDA2               0.4f             // Pole placement gain in sway
#define LAMBDA3               0.7f             // Pole placement gain in heave
#define LAMBDA4               4.0f             // Pole placement gain in yaw
#define UMAX                  50.0f            // Maximum control authority    
#define UMIN                  -50.0f           // Minimum control authority
#define C1                    3.0f             // Adaptive law parameter in surge
#define C2                    3.0f             // Adaptive law parameter in sway
#define C3                    1.0f             // Adaptive law parameter in heave
#define C4                    1.0f             // Adaptive law parameter in yaw
#define M                     10               // number of adaptive parameters
#define GAMMA                 10.0f            // adaptive law gain


class AC_NonLinearControl{
public:

    // Constructor
    AC_NonLinearControl(AP_AHRS_View & ahrs, const AP_InertialNav& inav,
                     const AP_Motors & motors, float dt);

    // Empty destructor to suppress compiler warning
    //virtual ~AC_NonLinearControl() {}

    // Initialize control
    void init_nonlin_control();

    // Run control
    void update_nonlin_control();

    // Update control output
    void update_output();

    // Update target
    void update_target();

    // Update state
    void update_state();

    // Update rotation amtrix
    void update_rot_matrix();

    // Convert a 321-intrinsic euler angle derivative to an angular velocity vector
    void euler_rate_to_ang_vel(const Vector3f& euler_rad, const Vector3f& euler_rate_rads, Vector3f& ang_vel_rads);

    // Convert an angular velocity vector to a 321-intrinsic euler angle derivative
    bool ang_vel_to_euler_rate(const Vector3f& euler_rad, const Vector3f& ang_vel_rads, Vector3f& euler_rate_rads);

    // convert a vector from body to earth frame
    Vector3f body_to_earth(const Vector3f &v) const ;

    // convert a vector from earth to body frame
    Vector3f earth_to_body(const Vector3f &v) const ;


protected:

    // Parameters
    float       _dt;                    // time difference (in seconds) between calls from the main program
    int         _N;                      // Model dof

    // Variables
    Vector4f _target;// target location in cm from home
    VVector4f _eta;           // _eta(1:3)  : AUV's position in NEU frame in cm relative to home (pos where UUV is armed)
                              // _eta(4:N)  : AUV's euler angles (rad) 
    Vector4f _nu;             // _nu(1:3)   : AUV's body fixed linear velocity
                              // _nu(4:N)   : AUV's body fixed angular velocity
    Vector4f _deta;           // _deta(1:3) : AUV's velocity in NEU frame (cm/s)
                              // _deta(4:N) : AUV's euler rate vector
    Vector4f _tau;   // control input (N) 

    // Transformation matrices
    Matrix3f _rot_mat         // Rotation matrix J: _deta(1:3) = _rot_mat * _nu(1:3)


    // references to control libraries
    AC_AFLC_4D AFLC;

    // references to inertial nav and ahrs libraries
    AP_AHRS_View &                  _ahrs;
    const AP_InertialNav &          _inav;
    const AP_Motors &               _motors; //AP_MotorsMulticopter &          _motors_multi; 




}