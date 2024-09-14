cmake_minimum_required(VERSION 3.25)

project(gpsClient)


add_executable(${PROJECT_NAME} cJSON.c gps_send_data.c)

target_link_libraries(${PROJECT_NAME} -lgps)
target_link_libraries(${PROJECT_NAME} -lcurl)

