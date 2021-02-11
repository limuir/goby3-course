subnet_mask=0xFF00
subnet_index={'satellite': 0, 'acomms': 1}
num_modems_in_subnet=(0xFFFF & subnet_mask)+1

# Broadcast is modem id = 0 in Goby, so increment vehicle id by 1 to get base modem id
def base_modem_id(vehicle_id):
    return vehicle_id + 1

def satellite_modem_id(vehicle_id):
    return base_modem_id(vehicle_id) + subnet_index['satellite']*num_modems_in_subnet

def acomms_modem_id(vehicle_id):
    return base_modem_id(vehicle_id) + subnet_index['acomms']*num_modems_in_subnet

# first id is usv id
usv_vehicle_id=1
acomms_first_modem_id=acomms_modem_id(usv_vehicle_id)
def acomms_mac_slots(number_of_auvs):
    # one usv
    number_of_acomms_modems = 1 + number_of_auvs
    slots=''
    for i in range(acomms_first_modem_id, acomms_first_modem_id + number_of_acomms_modems):
        slots += 'slot { src: ' + str(i) + ' slot_seconds: 10 max_frame_bytes: 128 }\n'
    return slots
