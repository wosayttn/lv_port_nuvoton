"""
Autobuild configuration for lv_port_nuvoton CI scripts.

All tool paths, project scan roots, and per-toolchain settings are
centralised here so that individual build scripts remain concise.
"""

# ---------------------------------------------------------------------------
# Project scan roots (relative to tools/autobuild/)
# ---------------------------------------------------------------------------
PROJ_FOLDER_ROOTS = [
    '..\\..\\board',
    '..\\..\\misc',
]

# ---------------------------------------------------------------------------
# Keil MDK
# ---------------------------------------------------------------------------
KEIL_UV4_PATH = "C:\\Keil_v5\\UV4\\Uv4.exe"
KEIL_STATE_FILE = 'keil_build_state.json'
KEIL_LOG_FILE = 'keil.txt'
KEIL_PROJECT_EXTENSIONS = ('*.uvproj', '*.uvprojx')
KEIL_BLACKLIST = ['warning:', 'Warning: ', 'error: ', 'Error: ']
KEIL_WHITELIST = [
    '[-Wlicense-management]',
    'Q9931W:',
    'Your license ',
    ' does not support the selected User Based Licensing technology',
]

# ---------------------------------------------------------------------------
# IAR Embedded Workbench
# ---------------------------------------------------------------------------
IAR_EXE_PATH = "C:\\Program Files\\IAR Systems\\Embedded Workbench 9.2\\common\\bin\\iarbuild.exe"
IAR_STATE_FILE = 'iar_build_state.json'
IAR_LOG_FILE = 'iar.txt'
IAR_PROJECT_EXTENSION = '*.ewp'

# ---------------------------------------------------------------------------
# NuEclipse (GCC)
# ---------------------------------------------------------------------------
NUECLIPSE_EXE_PATH = (
    "C:\\Program Files (x86)\\Nuvoton Tools\\NuEclipse"
    "\\V1.02.029r\\NuEclipse\\eclipse\\eclipsec.exe"
)
NUECLIPSE_STATE_FILE = 'gcc_build_state.json'
NUECLIPSE_LOG_FILE = 'gcc.txt'
NUECLIPSE_PROJECT_FILE = '*.cproject'
NUECLIPSE_BLACKLIST = ['[Fatal Error]', 'An error has occurred.', ' error: ', 'Error: ']
NUECLIPSE_LOG_SKIP_LINES = 4  # JVM deprecation warnings to skip

# ---------------------------------------------------------------------------
# VSCode / CMSIS csolution (vcpkg toolchain)
# ---------------------------------------------------------------------------
VCPKG_STATE_FILE = 'vcpkg_build_state.json'
VCPKG_LOG_FILE = 'vscode.txt'
VCPKG_PROJECT_PATTERN = '*.csolution.y*ml'
VCPKG_BLACKLIST = [
    '[Fatal Error]',
    'An error has occurred.',
    ' error: ',
    ' warning: ',
    'Error: ',
    'Warning: ',
    'warning csolution: ',
]

# ---------------------------------------------------------------------------
# Build timeout (seconds)
# ---------------------------------------------------------------------------
BUILD_TIMEOUT = 300
