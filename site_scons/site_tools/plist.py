"""
Tool to generate Info.plist.
"""

import os
from SCons.Script import Builder, Action


def generate_framework_plist(target, source, env):
    bundle_executable = env.get("bundle_executable", "MyFramework")
    bundle_identifier = env.get("bundle_identifier", "com.example.MyFramework")
    bundle_name = env.get("bundle_name", bundle_executable)
    bundle_version_string = env.get("bundle_version", "1.0")
    bundle_version = bundle_version_string.split("-", 1)[0]
    bundle_platforms = env.get("bundle_platforms", ["MacOSX"])
    bundle_package_type = env.get("bundle_package_type", "FMWK")  # FMWK or BNDL
    bundle_min_system = env.get("bundle_min_system", env.get("macos_deployment_target", "10.13"))

    platforms_content = "\n".join(f"        <string>{p}</string>" for p in bundle_platforms)

    content = f"""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>{bundle_executable}</string>
    <key>CFBundleIdentifier</key>
    <string>{bundle_identifier}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>{bundle_name}</string>
    <key>CFBundlePackageType</key>
    <string>{bundle_package_type}</string>
    <key>CFBundleShortVersionString</key>
    <string>{bundle_version_string}</string>
    <key>CFBundleSupportedPlatforms</key>
    <array>
{platforms_content}
    </array>
    <key>CFBundleVersion</key>
    <string>{bundle_version}</string>
    <key>LSMinimumSystemVersion</key>
    <string>{bundle_min_system}</string>
</dict>
</plist>"""

    with open(str(target[0]), "w") as f:
        f.write(content)

    return None


def generate(env):
    plist_builder = Builder(action=Action(generate_framework_plist, cmdstr="Generating Info.plist for $TARGET"))
    env.Append(BUILDERS={"FrameworkPlist": plist_builder})


def exists(env):
    return True
