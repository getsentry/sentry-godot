"""
Tool to generate framework Info.plist files.

Provides:
- FrameworkPlist builder: creates Info.plist as a SCons build target.
- WriteFrameworkPlist method: writes Info.plist directly (for use in actions).
"""

from SCons.Script import Builder, Action


def _framework_plist_content(
    bundle_executable,
    bundle_identifier=None,
    bundle_name=None,
    bundle_version="1.0",
    bundle_platforms=None,
    bundle_package_type="FMWK",
    min_os_version=None,
):
    """Generate Info.plist content string for a framework bundle."""
    if bundle_identifier is None:
        bundle_identifier = f"com.example.{bundle_executable}"
    if bundle_name is None:
        bundle_name = bundle_executable
    if bundle_platforms is None:
        bundle_platforms = ["MacOSX"]

    short_version = bundle_version.split("-", 1)[0]
    platforms_content = "\n".join(f"\t\t<string>{p}</string>" for p in bundle_platforms)

    # Use the appropriate minimum version key for the platform.
    is_ios = any(p in ("iPhoneOS", "iPhoneSimulator") for p in bundle_platforms)
    min_version_key = "MinimumOSVersion" if is_ios else "LSMinimumSystemVersion"
    min_version_entry = ""
    if min_os_version:
        min_version_entry = (
            f"\t<key>{min_version_key}</key>\n"
            f"\t<string>{min_os_version}</string>\n"
        )

    return f"""\
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
\t<key>CFBundleExecutable</key>
\t<string>{bundle_executable}</string>
\t<key>CFBundleIdentifier</key>
\t<string>{bundle_identifier}</string>
\t<key>CFBundleInfoDictionaryVersion</key>
\t<string>6.0</string>
\t<key>CFBundleName</key>
\t<string>{bundle_name}</string>
\t<key>CFBundlePackageType</key>
\t<string>{bundle_package_type}</string>
\t<key>CFBundleShortVersionString</key>
\t<string>{bundle_version}</string>
\t<key>CFBundleSupportedPlatforms</key>
\t<array>
{platforms_content}
\t</array>
\t<key>CFBundleVersion</key>
\t<string>{short_version}</string>
{min_version_entry}\
</dict>
</plist>
"""


def _generate_framework_plist_action(target, source, env):
    """SCons builder action: generates Info.plist from environment variables."""
    plist_keys = [
        "bundle_executable",
        "bundle_identifier",
        "bundle_name",
        "bundle_version",
        "bundle_platforms",
        "bundle_package_type",
        "min_os_version",
    ]
    kwargs = {key: env[key] for key in plist_keys if key in env}
    content = _framework_plist_content(**kwargs)
    with open(str(target[0]), "w") as f:
        f.write(content)


def write_framework_plist(env, path, **kwargs):
    """Write an Info.plist for a framework bundle.

    Can be called directly from SCons actions. Accepts the same keyword
    arguments as _framework_plist_content().
    """
    content = _framework_plist_content(**kwargs)
    with open(str(path), "w") as f:
        f.write(content)


def generate(env):
    plist_builder = Builder(
        action=Action(
            _generate_framework_plist_action,
            cmdstr="Generating Info.plist: $TARGET",
        )
    )
    env.Append(BUILDERS={"FrameworkPlist": plist_builder})
    env.AddMethod(write_framework_plist, "WriteFrameworkPlist")


def exists(env):
    return True
