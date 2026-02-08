if(NOT DEFINED OUTPUT_FILE OR OUTPUT_FILE STREQUAL "")
  message(FATAL_ERROR "OUTPUT_FILE is required.")
endif()

if(NOT DEFINED SYMBOL_NAME OR SYMBOL_NAME STREQUAL "")
  message(FATAL_ERROR "SYMBOL_NAME is required.")
endif()

get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

set(BYTE_COUNT 0)
set(HEX_DATA "")
if(DEFINED INPUT_FILE AND EXISTS "${INPUT_FILE}")
  file(READ "${INPUT_FILE}" HEX_DATA HEX)
  string(LENGTH "${HEX_DATA}" HEX_LENGTH)
  math(EXPR BYTE_COUNT "${HEX_LENGTH} / 2")
endif()

set(CPP_CONTENT "#include <cstddef>\n\n")
string(APPEND CPP_CONTENT "extern const unsigned char ${SYMBOL_NAME}[] = {\n")

if(BYTE_COUNT GREATER 0)
  math(EXPR LAST_INDEX "${BYTE_COUNT} - 1")
  set(LINE "  ")
  set(BYTES_ON_LINE 0)

  foreach(INDEX RANGE 0 ${LAST_INDEX})
    math(EXPR OFFSET "${INDEX} * 2")
    string(SUBSTRING "${HEX_DATA}" ${OFFSET} 2 BYTE_HEX)

    string(APPEND LINE "0x${BYTE_HEX}")
    if(NOT INDEX EQUAL LAST_INDEX)
      string(APPEND LINE ", ")
    endif()

    math(EXPR BYTES_ON_LINE "${BYTES_ON_LINE} + 1")
    if(BYTES_ON_LINE EQUAL 12 AND NOT INDEX EQUAL LAST_INDEX)
      string(APPEND CPP_CONTENT "${LINE}\n")
      set(LINE "  ")
      set(BYTES_ON_LINE 0)
    endif()
  endforeach()

  if(NOT LINE STREQUAL "  ")
    string(APPEND CPP_CONTENT "${LINE}\n")
  endif()
else()
  string(APPEND CPP_CONTENT "  0x00\n")
endif()

string(APPEND CPP_CONTENT "};\n\n")
string(APPEND CPP_CONTENT "extern const std::size_t ${SYMBOL_NAME}Size = ${BYTE_COUNT};\n")

file(WRITE "${OUTPUT_FILE}" "${CPP_CONTENT}")
