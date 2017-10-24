
(cl:in-package :asdf)

(defsystem "motionEstimationEKF-msg"
  :depends-on (:roslisp-msg-protocol :roslisp-utils :std_msgs-msg
)
  :components ((:file "_package")
    (:file "optical_flow_rad" :depends-on ("_package_optical_flow_rad"))
    (:file "_package_optical_flow_rad" :depends-on ("_package"))
    (:file "velocityMsg" :depends-on ("_package_velocityMsg"))
    (:file "_package_velocityMsg" :depends-on ("_package"))
  ))