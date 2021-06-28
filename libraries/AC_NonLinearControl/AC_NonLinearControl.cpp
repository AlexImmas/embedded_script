#include <eigen-3.3.9/Eigen/Dense>
//using namespace Eigen;

#include "AC_NonLinearControl.h"

// table of user settable parameters
const AP_Param::GroupInfo AC_NonLinearControl::var_info[] = {

   // AP_GROUPINFO("P",    0, AC_NonLinearControl, _dt, 0),

    AP_GROUPEND
};


AC_NonLinearControl::AC_NonLinearControl(AP_AHRS_View & ahrs, const AP_InertialNav& inav,
                                AP_Motors & motors, float dt, float u1, AC_WPNav& wpnav) :
        _dt(dt),
        _N(N),
        _u1(u1),
        AFLC(N, dt, BETA1, BETA2, BETA3, BETA4, LAMBDA1, LAMBDA2, LAMBDA3, LAMBDA4,
                 UMAX, UMIN, C1, C2, C3, C4, GAMMA),
        _ahrs(ahrs),
        _inav(inav),
        _motors(motors),
        _wpnav(wpnav)
        {
           AP_Param::setup_object_defaults(this, var_info);

        }


void AC_NonLinearControl::init_nonlin_control()
{


    // Euler angles in body-frame (b), angular velocity in body-frame (b)
    // and Euler rate in NEU-frame (n)
    Eigen::Vector3f ang_b;
    ang_b(0) = _ahrs.pitch ; ang_b(1) = _ahrs.roll; ang_b(2) = _ahrs.yaw;
    //Eigen::Vector3f ang_vel_b = _ahrs.get_gyro();
    Eigen::Vector3f ang_vel_b;
    ang_vel_b(0) = _ahrs.get_gyro()[0]; ang_vel_b(1) = _ahrs.get_gyro()[1]; ang_vel_b(2) = _ahrs.get_gyro()[2];
    Eigen::Vector3f ang_vel_n = Eigen::Vector3f::Zero(3);
    ang_vel_to_euler_rate(ang_b, ang_vel_b, ang_vel_n);

    // Position in NEU-frame (n), Velocity in NEU-frame (n) and in body-frame (b)
    //Eigen::Vector3f pos_n     = _inav.get_position();
    Eigen::Vector3f pos_n;
    pos_n(0) = _inav.get_position()[0]; pos_n(1) = _inav.get_position()[1]; pos_n(2) = _inav.get_position()[2];
    //Eigen::Vector3f pos_vel_n = _inav.get_velocity();
    Eigen::Vector3f pos_vel_n;
    pos_vel_n(0) = _inav.get_velocity()[0]; pos_vel_n(1) = _inav.get_velocity()[1]; pos_vel_n(2) = _inav.get_velocity()[2];

    //// Relax controller - Set target to current position
    _target(0) = pos_n(0);
    _target(1) = pos_n(1);
    _target(2) = pos_n(2);
    //_target(3) = ang_b(0);
    //_target(4) = ang_b(1);
    _target(3) = ang_b(2);

    //// Building reference model matrices and initialize reference states
    // Define eta_r0
    Eigen::Vector4f eta_r0;
    eta_r0(0) = pos_n(0);
    eta_r0(1) = pos_n(1);
    eta_r0(2) = pos_n(2);
    //eta_r0(3) = ang_b(0);
    //eta_r0(4) = ang_b(1);
    eta_r0(3) = ang_b(2);


    // Define deta_r0
    Eigen::Vector4f deta_r0;
    deta_r0(0) = pos_vel_n(0);
    deta_r0(1) = pos_vel_n(1);
    deta_r0(2) = pos_vel_n(2);
    //deta_r0(3) = TBD
    //deta_r0(4) = TBD
    deta_r0(3) =  ang_vel_n(2);

    AFLC.init_reference_model(eta_r0, deta_r0);
}

void AC_NonLinearControl::update_state()
{
    // Euler angles in body-frame (b), angular velocity in body-frame (b)
    // and Euler rate in NEU-frame (n)
    Eigen::Vector3f ang_b;
    ang_b(0) = _ahrs.pitch; ang_b(1) = _ahrs.roll; ang_b(2) = _ahrs.yaw;
    //Eigen::Vector3f ang_vel_b = _ahrs.get_gyro();
    Eigen::Vector3f ang_vel_b;
    ang_vel_b(0) = _ahrs.get_gyro()[0]; ang_vel_b(1) = _ahrs.get_gyro()[1]; ang_vel_b(2) = _ahrs.get_gyro()[2];
    Eigen::Vector3f ang_vel_n = Eigen::Vector3f::Zero(3);
    ang_vel_to_euler_rate(ang_b, ang_vel_b, ang_vel_n);

    // Position in NEU-frame (n), Velocity in NEU-frame (n) and in body-frame (b)
    //Eigen::Vector3f pos_n     = _inav.get_position();
    Eigen::Vector3f pos_n;
    pos_n(0) = _inav.get_position()[0]; pos_n(1) = _inav.get_position()[1]; pos_n(2) = _inav.get_position()[2];

    Vector3f pos_vel_n_meas = _inav.get_velocity();
    update_rot_matrix(ang_b(1), ang_b(0), ang_b(2));
    Vector3f pos_vel_b_meas = earth_to_body(pos_vel_n_meas);
    Eigen::Vector3f pos_vel_n;
    pos_vel_n(0) = pos_vel_n_meas[0]; pos_vel_n(1) = pos_vel_n_meas[1]; pos_vel_n(2) = pos_vel_n_meas[2];
    Eigen::Vector3f pos_vel_b;
    pos_vel_b(0) = pos_vel_b_meas[0]; pos_vel_b(1) = pos_vel_b_meas[1]; pos_vel_b(2) = pos_vel_b_meas[2];


    // Define _eta
    _eta(0) = pos_n(0);
    _eta(1) = pos_n(1);
    _eta(2) = pos_n(2);
    //_eta(3) = ang_b(0);
    //_eta(4) = ang_b(1);
    _eta(3) = ang_b(2);

    // Define _nu
    _nu(0) = pos_vel_b(0);
    _nu(1) = pos_vel_b(1);
    _nu(2) = pos_vel_b(2);
    //_nu(3) = ang_vel_b(0);
    //_nu(4) = ang_vel_b(1);
    _nu(3) = ang_vel_b(2);

    // Define _deta
    _deta(0) = pos_vel_n(0);
    _deta(1) = pos_vel_n(1);
    _deta(2) = pos_vel_n(2);
    //_deta(3) = TBD
    //_deta(4) = TBD
    _deta(3) =  ang_vel_n(2);
}

void AC_NonLinearControl::update_nonlin_control()
{
      // Update transformation matrices
      AFLC.update_transformation_matrices(_eta, _nu);
      // Update reference position
      _target = self.update_target();
      AFLC.update_reference_model(_target);
      //  Anti windup
      AFLC.update_anti_windup(_eta);
      // Update commanded acceleration
      AFLC.update_commanded_acceleration(_eta, _deta, _nu);
      // Update parameters law
      AFLC.update_parameters_law(_eta, _deta, _nu);
      // Compute control input
      AFLC.update_control_input();
      // return control inputs
      _tau = AFLC.get_control_input();

}

void AC_NonLinearControl::update_output()
{

    // Scale control inputs between [-1,1] except heave in [0,1]
    _tau = _tau/_u1;
    _tau(2) = (_tau(2)+1)/2;

    _motors.set_forward(_tau(0));
    _motors.set_lateral(_tau(1));
    _motors.set_throttle(_tau(2));
    _motors.set_pitch(0);
    _motors.set_roll(0);
    _motors.set_yaw(_tau(3));

}

const Vector3f AC_NonLinearControl::update_target()
{
    Eigen::Vector3f target3d;
    target3d = _wpnav.get_wp_destination();

    Eigen::Vector3f ang_b;
    ang_b(0) = _ahrs.pitch ;
    ang_b(1) = _ahrs.roll;
    ang_b(2) = _ahrs.yaw;

    Eigen::Vector4f _target;
    _target(0) = target3d(0);
    _target(1) = target3d(1);
    _target(2) = target3d(2);
    _target(3) = ang_b(2);

    return _target
}





void AC_NonLinearControl::update_rot_matrix(float & roll, float & pitch, float & yaw)
{
    _rot_mat.from_euler(roll, pitch, yaw);
}

// Convert a 321-intrinsic euler angle derivative to an angular velocity vector
void AC_NonLinearControl::euler_rate_to_ang_vel(const Eigen::Vector3f& euler_rad, const Eigen::Vector3f& euler_rate_rads, Eigen::Vector3f& ang_vel_rads)
{
    float sin_theta = sinf(euler_rad(1));
    float cos_theta = cosf(euler_rad(1));
    float sin_phi = sinf(euler_rad(0));
    float cos_phi = cosf(euler_rad(0));

    ang_vel_rads(0) = euler_rate_rads(0) - sin_theta * euler_rate_rads(2);
    ang_vel_rads(1) = cos_phi * euler_rate_rads(1) + sin_phi * cos_theta * euler_rate_rads(2);
    ang_vel_rads(2) = -sin_phi * euler_rate_rads(1) + cos_theta * cos_phi * euler_rate_rads(2);
}

// Convert an angular velocity vector to a 321-intrinsic euler angle derivative
// Returns false if the vehicle is pitched 90 degrees up or down
bool AC_NonLinearControl::ang_vel_to_euler_rate(const Eigen::Vector3f& euler_rad, const Eigen::Vector3f& ang_vel_rads, Eigen::Vector3f& euler_rate_rads)
{
    float sin_theta = sinf(euler_rad(1));
    float cos_theta = cosf(euler_rad(1));
    float sin_phi = sinf(euler_rad(0));
    float cos_phi = cosf(euler_rad(0));

    // When the vehicle pitches all the way up or all the way down, the euler angles become discontinuous. In this case, we just return false.
    if (is_zero(cos_theta)) {
        return false;
    }

    euler_rate_rads(0) = ang_vel_rads(0) + sin_phi * (sin_theta / cos_theta) * ang_vel_rads(1) + cos_phi * (sin_theta / cos_theta) * ang_vel_rads(2);
    euler_rate_rads(1) = cos_phi * ang_vel_rads(1) - sin_phi * ang_vel_rads(2);
    euler_rate_rads(2) = (sin_phi / cos_theta) * ang_vel_rads(1) + (cos_phi / cos_theta) * ang_vel_rads(2);
    return true;
}

// convert a vector from body to earth frame
Vector3f AC_NonLinearControl::body_to_earth(const Vector3f &v) const
{
        return v * _rot_mat;
}

    // convert a vector from earth to body frame
Vector3f AC_NonLinearControl::earth_to_body(const Vector3f &v) const
{
        return _rot_mat.mul_transpose(v) ;
}
