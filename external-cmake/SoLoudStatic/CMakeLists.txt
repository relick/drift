## don't think this is needed? came from the autogen
##include(GNUInstallDirs)
set(SOLOUD_SRC_DIR ${SRC_DIR}/soloud/src)

set(SOLOUD_SOURCE
  ${SOLOUD_SRC_DIR}/audiosource/monotone/soloud_monotone.cpp
  ${SOLOUD_SRC_DIR}/audiosource/noise/soloud_noise.cpp
  ${SOLOUD_SRC_DIR}/audiosource/openmpt/soloud_openmpt.cpp
  ${SOLOUD_SRC_DIR}/audiosource/openmpt/soloud_openmpt_dll.c
  ${SOLOUD_SRC_DIR}/audiosource/sfxr/soloud_sfxr.cpp
  ${SOLOUD_SRC_DIR}/audiosource/speech/darray.cpp
  ${SOLOUD_SRC_DIR}/audiosource/speech/klatt.cpp
  ${SOLOUD_SRC_DIR}/audiosource/speech/resonator.cpp
  ${SOLOUD_SRC_DIR}/audiosource/speech/soloud_speech.cpp
  ${SOLOUD_SRC_DIR}/audiosource/speech/tts.cpp
  ${SOLOUD_SRC_DIR}/audiosource/tedsid/sid.cpp
  ${SOLOUD_SRC_DIR}/audiosource/tedsid/soloud_tedsid.cpp
  ${SOLOUD_SRC_DIR}/audiosource/tedsid/ted.cpp
  ${SOLOUD_SRC_DIR}/audiosource/vic/soloud_vic.cpp
  ${SOLOUD_SRC_DIR}/audiosource/vizsn/soloud_vizsn.cpp
  ${SOLOUD_SRC_DIR}/audiosource/wav/dr_impl.cpp
  ${SOLOUD_SRC_DIR}/audiosource/wav/soloud_wav.cpp
  ${SOLOUD_SRC_DIR}/audiosource/wav/soloud_wavstream.cpp
  ${SOLOUD_SRC_DIR}/audiosource/wav/stb_vorbis.c
  ${SOLOUD_SRC_DIR}/backend/null/soloud_null.cpp
  ${SOLOUD_SRC_DIR}/backend/winmm/soloud_winmm.cpp
  ${SOLOUD_SRC_DIR}/core/soloud.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_audiosource.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_bus.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_core_3d.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_core_basicops.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_core_faderops.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_core_filterops.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_core_getters.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_core_setters.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_core_voicegroup.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_core_voiceops.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_fader.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_fft.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_fft_lut.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_file.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_filter.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_misc.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_queue.cpp
  ${SOLOUD_SRC_DIR}/core/soloud_thread.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_bassboostfilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_biquadresonantfilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_dcremovalfilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_echofilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_fftfilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_flangerfilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_freeverbfilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_lofifilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_robotizefilter.cpp
  ${SOLOUD_SRC_DIR}/filter/soloud_waveshaperfilter.cpp
)

add_library(SoLoud STATIC ${SOLOUD_SOURCE})

target_include_directories(SoLoud PRIVATE
  ${SOLOUD_SRC_DIR}/audiosource
  ${SOLOUD_SRC_DIR}/backend
  ${SOLOUD_SRC_DIR}/core
  ${SOLOUD_SRC_DIR}/c_api
  ${SOLOUD_SRC_DIR}/filter
  ${SOLOUD_SRC_DIR}/tools
  ${SOLOUD_SRC_DIR}/audiosource/monotone
  ${SOLOUD_SRC_DIR}/audiosource/noise
  ${SOLOUD_SRC_DIR}/audiosource/openmpt
  ${SOLOUD_SRC_DIR}/audiosource/sfxr
  ${SOLOUD_SRC_DIR}/audiosource/speech
  ${SOLOUD_SRC_DIR}/audiosource/tedsid
  ${SOLOUD_SRC_DIR}/audiosource/vic
  ${SOLOUD_SRC_DIR}/audiosource/vizsn
  ${SOLOUD_SRC_DIR}/audiosource/wav
  ${SOLOUD_SRC_DIR}/backend/alsa
  ${SOLOUD_SRC_DIR}/backend/coreaudio
  ${SOLOUD_SRC_DIR}/backend/jack
  ${SOLOUD_SRC_DIR}/backend/miniaudio
  ${SOLOUD_SRC_DIR}/backend/nosound
  ${SOLOUD_SRC_DIR}/backend/null
  ${SOLOUD_SRC_DIR}/backend/openal
  ${SOLOUD_SRC_DIR}/backend/opensles
  ${SOLOUD_SRC_DIR}/backend/oss
  ${SOLOUD_SRC_DIR}/backend/portaudio
  ${SOLOUD_SRC_DIR}/backend/sdl
  ${SOLOUD_SRC_DIR}/backend/sdl2_static
  ${SOLOUD_SRC_DIR}/backend/sdl_static
  ${SOLOUD_SRC_DIR}/backend/vita_homebrew
  ${SOLOUD_SRC_DIR}/backend/wasapi
  ${SOLOUD_SRC_DIR}/backend/winmm
  ${SOLOUD_SRC_DIR}/backend/xaudio2
  ${SOLOUD_SRC_DIR}/tools/codegen
  ${SOLOUD_SRC_DIR}/tools/lutgen
  ${SOLOUD_SRC_DIR}/tools/resamplerlab
  ${SOLOUD_SRC_DIR}/tools/sanity
  ${SOLOUD_SRC_DIR}/tools/tedsid2dump
)
target_include_directories(SoLoud SYSTEM PUBLIC ${SRC_DIR}/soloud/include)
target_compile_definitions(SoLoud PRIVATE
  _CRT_SECURE_NO_WARNINGS
  WITH_WINMM
  WITH_NULL
  $<$<CONFIG:Debug,RelWithDebInfo>:
    DEBUG
  >
  $<$<CONFIG:Release>:
    NDEBUG
  >
)

target_compile_options(SoLoud PRIVATE
  $<$<CXX_COMPILER_ID:Clang,GNU>:
    -m64
    $<IF:$<BOOL:${MSVC}>,
      /EHs-c- /GR-,
      -fno-exceptions -fno-rtti
    >
    -Wno-missing-braces -Wno-unused-value -Wno-unused-variable -Wno-misleading-indentation -Wno-char-subscripts
  >
  $<$<CXX_COMPILER_ID:GNU>:
    -MMD -MP
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /EHs-c- /GR- /noover
  >
)

install(TARGETS SoLoud RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
