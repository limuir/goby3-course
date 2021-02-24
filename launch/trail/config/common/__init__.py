import os
import sys
import goby

def check_args():
    if len(sys.argv) >= 2:
        app=sys.argv[1]
    else:
        goby.config.fail('App name must be given as first command line argument')

check_args()
app=sys.argv[1]

goby3_course_root=os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + '/../../../../../goby3-course')
goby.config.checkdir(goby3_course_root)

goby3_course_lib_dir=os.path.normpath(goby3_course_root + '/build/lib')
goby.config.checkdir(goby3_course_lib_dir)

goby3_course_messages_lib=goby3_course_lib_dir+'/libgoby3_course_messages.so'
goby.config.checkfile(goby3_course_messages_lib)

goby3_course_templates_dir=os.path.normpath(os.path.dirname(os.path.realpath(__file__)) +  '/../templates')
goby.config.checkdir(goby3_course_templates_dir)

goby3_course_logs_dir=os.path.normpath(goby3_course_root +  '/logs')
goby.config.checkdir(goby3_course_logs_dir)

