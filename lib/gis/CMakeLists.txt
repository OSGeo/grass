set(gislib_SRCS
  adj_cellhd.c
  copy_dir.c
  get_ellipse.c             ll_scan.c                 open_misc.c               proj3.c                   units.c
  alloc.c                   copy_file.c               get_projinfo.c            locale.c                  overwrite.c               put_window.c              user_config.c
  area.c                    counter.c                 get_window.c              location.c                pager.c                   putenv.c                  verbose.c
  area_ellipse.c            date.c                    getl.c                    lrand48.c                 parser.c                  radii.c                   view.c
  area_poly1.c              datum.c                   gisbase.c                 ls.c                      parser_dependencies.c     rd_cellhd.c               whoami.c
  area_poly2.c              debug.c                   gisdbase.c                ls_filter.c               parser_help.c             remove.c                  win32_pipes.c
  area_sphere.c             distance.c                gisinit.c                 lz4.c                     parser_html.c             rename.c                  wind_2_box.c
  ascii_chk.c               done_msg.c                handler.c                 mach_name.c               parser_interface.c        rhumbline.c               wind_format.c
  asprintf.c                endian.c                  home.c                    make_loc.c                parser_rest.c             rotate.c                  wind_in.c
  basename.c                env.c                     ilist.c                   make_mapset.c             parser_script.c           seek.c                    wind_limits.c
  bres_line.c               error.c                   intersect.c               mapcase.c                 parser_standard_options.c set_window.c              wind_overlap.c
  clicker.c                 file_name.c               is.c                      mapset.c                  parser_wps.c              short_way.c               wind_scan.c
  cmprbzip.c                find_etc.c                key_value1.c              mapset_msc.c              paths.c                   sleep.c                   window_map.c
  cmprlz4.c                 find_file.c               key_value2.c              mapset_nme.c              percent.c                 snprintf.c                worker.c
  cmprrle.c                 find_rast.c               key_value3.c              mkstemp.c                 plot.c                    spawn.c                   wr_cellhd.c
  cmprzlib.c                find_rast3d.c             key_value4.c              myname.c                  pole_in_poly.c            strings.c                 writ_zeros.c
  color_rules.c             find_vect.c               legal_name.c              named_colr.c              popen.c                   tempfile.c                xdr.c
  color_str.c                                  line_dist.c               nl_to_spaces.c            progrm_nme.c              timestamp.c               zero.c
  commas.c                  geodesic.c                list.c                    nme_in_mps.c              proj1.c                   token.c                   zone.c
  compress.c                geodist.c                 ll_format.c               open.c                    proj2.c                   trim_dec.c
  )

if(MINGW)
  list(APPEND gislib_SRCS "fmode.c")
endif()
set(GRASS_VERSION_DATE 20160402)

build_module(NAME grass_gis SOURCES "${gislib_SRCS}"
  DEPENDS ZLIB LIBM grass_datetime
  DEFS "-DGRASS_VERSION_DATE=${GRASS_VERSION_DATE}"
  )
