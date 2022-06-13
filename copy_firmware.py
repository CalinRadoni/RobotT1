import shutil

Import("env")

def copy_firmware_file(source, target, env):
    shutil.copy(str(target[0]), "firmware/")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware_file)
