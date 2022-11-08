add_library(farbot INTERFACE include/farbot/AsyncCaller.hpp
                             include/farbot/fifo.hpp
                             include/farbot/RealtimeObject.hpp
                             include/farbot/RealtimeTraits.hpp)

target_include_directories(farbot INTERFACE include)
