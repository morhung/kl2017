cmake_minimum_required(VERSION 2.8)
project(PiCapture)
SET(COMPILE_DEFINITIONS -Werror)
find_package( OpenCV REQUIRED )
include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)
include_directories("${PROJECT_SOURCE_DIR}/others")
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/face_rec")
include_directories("${PROJECT_SOURCE_DIR}/face_rec/include")
include_directories("${PROJECT_SOURCE_DIR}/face_rec/src")

set (FACE_REC
   face_rec/src/facerec.cpp
   face_rec/src/colormap.cpp
   face_rec/src/decomposition.cpp
   face_rec/src/helper.cpp
   face_rec/src/lbp.cpp
   face_rec/src/subspace.cpp)

link_directories(/opt/vc/lib)
add_executable(main_face_body_upper src/main.cpp src/PiCapture.cpp src/PeopleDetector.cpp ${FACE_REC})
target_link_libraries(main_face_body_upper mmal_core mmal_util mmal_vc_client bcm_host pthread wiringPi ${OpenCV_LIBS})
SET(CMAKE_BUILD_TYPE Release)