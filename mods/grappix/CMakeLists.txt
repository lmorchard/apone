cmake_minimum_required(VERSION 2.8.4)
project(cpp-mods.grappix)

file(GLOB MAIN_FILES grappix/*.cpp grappix/gui/*.cpp)
file(GLOB FREETYPE_FILES grappix/freetype-gl/*.c)
set(SOURCE_FILES ${MAIN_FILES} ${FREETYPE_FILES})

file(GLOB SPECIFIC_FILES grappix/specific/*.cpp)
add_custom_target(grappix.specific_files SOURCES ${SPECIFIC_FILES})

if(NOT TARGET image)
  add_subdirectory(../image image)
endif()

if(NOT TARGET tween)
  add_subdirectory(../tween tween)
endif()

if(NOT TARGET coreutils)
	add_subdirectory(../coreutils coreutils)
endif()

if(NOT TARGET freetype)
  add_subdirectory(../freetype freetype)
endif()
include_directories(${FREETYPE_INCLUDE_DIR})


if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(CORE_VIDEO_LIBRARY CoreVideo)
  find_library(IOKIT_LIBRARY IOKit)
  set(EXTRA_LIBS ${EXTRA_LIBS} ${COCOA_LIBRARY} ${CORE_VIDEO_LIBRARY} ${IOKIT_LIBRARY})
endif()

if(EMSCRIPTEN)
  
  set(SOURCE_FILES ${SOURCE_FILES} grappix/specific/window_pc.cpp)
  set(EXTRA_LIBS ${EXTRA_LIBS} -lGL)

elseif(RASPBERRY)

  set(VC ${PI_SYS_ROOT}/vc)
  set(SOURCE_FILES ${SOURCE_FILES} grappix/specific/window_pi.cpp grappix/specific/eglutil.cpp)
  set(EXTRA_LIBS ${EXTRA_LIBS} EGL GLESv2 vcos vchiq_arm bcm_host)
  include_directories(${VC}/include ${VC}/include/interface/vcos/pthreads ${VC}/include/interface/vmcs_host/linux)

elseif(WIN32)

  set(SOURCE_FILES ${SOURCE_FILES} grappix/specific/window_pc.cpp)

  find_package(OpenGL REQUIRED)
  find_package(glew REQUIRED)
  find_library(GLFW_LIBRARY NAMES libglfw3.a glfw3 glfw3dll REQUIRED)

#  find_path(GLFW_PATH GLFW/glfw3.h)
#  include_directories(${GLFW_PATH})
#  message(${GLFW_PATH})
  #
  set(EXTRA_LIBS ${EXTRA_LIBS} ${OPENGL_LIBRARIES})
  include_directories(${OPENGL_INCLUDE_DIRS})

  set(EXTRA_LIBS ${EXTRA_LIBS} ${GLEW_LIBRARIES})
  include_directories(${GLEW_INCLUDE_DIRS})

  set(EXTRA_LIBS ${EXTRA_LIBS} ${GLFW_LIBRARY})


elseif(UNIX)
  set(SOURCE_FILES ${SOURCE_FILES} grappix/specific/window_pc.cpp)
  find_package(OpenGL REQUIRED)

  if(APPLE)
    find_library(GLFW_LIBRARY NAMES glfw REQUIRED)
  else() # Ubuntu changed naming between 14.0 and 14.10
    find_library(GLFW_LIBRARY NAMES libglfw.so.3 libglfw3.a REQUIRED)
  endif()

  set(EXTRA_LIBS ${EXTRA_LIBS} ${OPENGL_LIBRARIES})
  include_directories(${OPENGL_INCLUDE_DIRS})
  set(EXTRA_LIBS ${EXTRA_LIBS} ${GLFW_LIBRARY})
  find_library(GLEW_LIBRARY GLEW REQUIRED)
  set(EXTRA_LIBS ${EXTRA_LIBS} ${GLEW_LIBRARY})

  if(NOT APPLE)
    if(GLFW_LIBRARY MATCHES "\\.a")
      message("GLFW is static")
      set(EXTRA_LIBS ${EXTRA_LIBS} -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lXinerama -lXcursor)
    else()
      message("GLFW is dynamic")
      set(EXTRA_LIBS ${EXTRA_LIBS} -lX11 -lXxf86vm -lpthread)
    endif()
  endif()

else()
endif()

add_definitions(-DWITH_FREETYPE)

file(GLOB SHADERS grappix/shaders/*.glsl)

add_custom_target(grappix.shaders SOURCES ${SHADERS})

file(MAKE_DIRECTORY .shaderx)
file(MAKE_DIRECTORY .shader)
foreach(SHADER ${SHADERS})
	message(${SHADER})
	get_filename_component(SHADERNAME ${SHADER} NAME)
	add_custom_command(OUTPUT ${SHADERNAME}.c
		COMMAND ${CMAKE_COMMAND} -E copy ${SHADER} .shader/${SHADERNAME}
		COMMAND xxd -i .shader/${SHADERNAME} > ${SHADERNAME}.c
		DEPENDS ${SHADER})
	set(SOURCE_FILES ${SOURCE_FILES} ${SHADERNAME}.c)
endforeach()

include_directories(.. grappix/freetype-gl)

add_library(grappix ${SOURCE_FILES})
target_link_libraries(grappix coreutils image tween freetype ${EXTRA_LIBS})
target_include_directories(grappix PRIVATE ..)
