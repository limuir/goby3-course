#!/usr/bin/env python3

# Generates Goby3 protobuf configuration using definitions and text substitution
# Usage: python3 example.pb.cfg.py app_name

import sys
import os
from goby import config
import common, common.origin, common.vehicle, common.comms

debug_log_file_dir = '/tmp/auv'
os.makedirs(debug_log_file_dir, exist_ok=True)
templates_dir=common.goby3_course_templates_dir

try:
    number_of_auvs=int(os.environ['goby3_course_n_auvs'])
except:
    config.fail('Must set goby3_course_n_auvs environmental variable')

try:
    auv_index=int(os.environ['goby3_course_auv_index'])
except:
    config.fail('Must set goby3_course_auv_index environmental variable')

    
vehicle_id=auv_index+common.comms.usv_vehicle_id+1
vehicle_type = 'AUV'
acomms_modem_id = common.comms.acomms_modem_id(vehicle_id)

app_common = config.template_substitute(templates_dir+'/_app.pb.cfg.in',
                                 app=common.app,
                                 tty_verbosity = 'QUIET',
                                 log_file_dir = debug_log_file_dir,
                                 log_file_verbosity = 'QUIET',
                                 warp=common.warp,
                                 lat_origin=common.origin.lat(),
                                 lon_origin=common.origin.lon())

interprocess_common = config.template_substitute(templates_dir+'/_interprocess.pb.cfg.in',
                                                 platform='auv')

link_acomms_block = config.template_substitute(templates_dir+'/_link_acomms.pb.cfg.in',
                                               subnet_mask=common.comms.subnet_mask,
                                               modem_id=acomms_modem_id,
                                               mac_slots=common.comms.acomms_mac_slots(number_of_auvs))
link_block=link_acomms_block

if common.app == 'gobyd':    
    print(config.template_substitute(templates_dir+'/gobyd.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common,
                                     link_block=link_block))
elif common.app == 'goby_frontseat_interface_basic_simulator':
    print(config.template_substitute(templates_dir+'/frontseat.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common,
                                     sim_start_lat = common.origin.lat(),
                                     sim_start_lon = common.origin.lon(),
                                     sim_port = common.vehicle.simulator_port(vehicle_id)))
elif common.app == 'goby_liaison':
    print(config.template_substitute(templates_dir+'/liaison.pb.cfg.in',
                              app_block=app_common,
                              interprocess_block = interprocess_common,
                              http_port=50002,
                              goby3_course_messages_lib=common.goby3_course_messages_lib))
elif common.app == 'goby3_course_nav_manager':
    print(config.template_substitute(templates_dir+'/nav_manager.pb.cfg.in',
                                     app_block=app_common,
                                     interprocess_block = interprocess_common,
                                     vehicle_type=vehicle_type,
                                     vehicle_id=vehicle_id,
                                     subscribe_to_vehicle_ids=''))
elif common.app == 'moos':
    print(config.template_substitute(templates_dir+'/auv.moos.in',
                                     warp=common.warp,
                                     lat_origin=common.origin.lat(),
                                     lon_origin=common.origin.lon(),
                                     bhv_file=templates_dir+'/auv.bhv'))
elif common.app == 'frontseat_sim':
    print(common.vehicle.simulator_port(vehicle_id))
else:
    sys.exit('App: {} not defined'.format(common.app))
