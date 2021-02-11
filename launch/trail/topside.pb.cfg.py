#!/usr/bin/env python3

# Generates Goby3 protobuf configuration using definitions and text substitution
# Usage: python3 example.pb.cfg.py app_name

import sys
import os
from goby import config
import common, common.origin, common.topside, common.comms

debug_log_file_dir = '/tmp/topside'
os.makedirs(debug_log_file_dir, exist_ok=True)

vehicle_id = 0 
satellite_modem_id = common.comms.satellite_modem_id(vehicle_id)
vehicle_type= 'TOPSIDE'

app_common = config.template_substitute('templates/_app.pb.cfg.in',
                                        app=common.app,
                                        tty_verbosity = 'QUIET',
                                        log_file_dir = debug_log_file_dir,
                                        log_file_verbosity = 'QUIET',
                                        warp=common.warp,
                                        lat_origin=common.origin.lat(),
                                        lon_origin=common.origin.lon())

interprocess_common = config.template_substitute('templates/_interprocess.pb.cfg.in',
                                                 platform='topside')


link_satellite_block = config.template_substitute('templates/_link_satellite.pb.cfg.in',
                                                  subnet_mask=common.comms.subnet_mask,
                                                  modem_id=satellite_modem_id)


if common.app == 'gobyd':    
    print(config.template_substitute('templates/gobyd.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common,
                                     link_block=link_satellite_block))
elif common.app == 'goby_geov_interface':
    print(config.template_substitute('templates/goby_geov_interface.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common))
elif common.app == 'goby_opencpn_interface':
    print(config.template_substitute('templates/goby_opencpn_interface.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common))
elif common.app == 'goby_liaison':
    print(config.template_substitute('templates/liaison.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common,
                                     http_port=50001,
                                     goby3_course_messages_lib=common.goby3_course_messages_lib))
elif common.app == 'goby3_course_nav_manager':
    print(config.template_substitute('templates/nav_manager.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common,
                                     vehicle_type=vehicle_type,
                                     vehicle_id=vehicle_id,
                                     subscribe_to_vehicle_ids='subscribe_to_vehicle_id: [2]'))
else:
    sys.exit('App: {} not defined'.format(common.app))
