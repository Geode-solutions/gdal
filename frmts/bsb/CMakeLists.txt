add_gdal_driver(TARGET gdal_BSB SOURCES bsb_read.h bsb_read.c bsbdataset.cpp
  PLUGIN_CAPABLE
  NO_DEPS
  NO_CXX_WFLAGS_EFFCXX
  NO_WFLAG_OLD_STYLE_CAST
)
gdal_standard_includes(gdal_BSB)

if (NOT GDAL_ENABLE_DRIVER_BSB_PLUGIN)
  add_executable(bsb2raw EXCLUDE_FROM_ALL bsb2raw.c)
  target_include_directories(bsb2raw PRIVATE $<TARGET_PROPERTY:gdal_BSB,SOURCE_DIR>)
  target_link_libraries(bsb2raw PRIVATE $<TARGET_NAME:${GDAL_LIB_TARGET_NAME}> gdal_BSB)
endif ()
