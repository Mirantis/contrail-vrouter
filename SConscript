#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

import subprocess
import sys
import os
import copy
import re
import platform

AddOption('--kernel-dir', dest = 'kernel-dir', action='store',
          help='Linux kernel source directory for vrouter.ko')

AddOption('--system-header-path', dest = 'system-header-path', action='store',
          help='Linux kernel headers for applications')

AddOption('--add-opts', dest = 'add-opts', action='store',
          help='Additional options for vrouter compilation')

env = DefaultEnvironment().Clone()
VRouterEnv = env
dpdk_exists = os.path.isdir('../third_party/dpdk')

# Get compile-time machine flags and define vrouter macros.
# Developers can check these compile-time flags to use
# specific CPU features.
compiler = env['CC']

if compiler == 'cl':
    env.Append(CCFLAGS = '/WX')

# get CPU flag for GCC
if compiler == 'gcc' or compiler == 'clang':
    target = env.get('TARGET_MACHINE')
    # specify gcc -march flag for x86_64
    if target == 'x86_64':
        cpu = env.get('CPU_TYPE')
        if cpu == 'native':
            env.Append(CCFLAGS = '-march=' + 'native')
        elif cpu == 'snb':
            env.Append(CCFLAGS = '-march=' + 'corei7-avx')
        elif cpu == 'ivb':
            env.Append(CCFLAGS = '-march=' + 'core-avx-i')
        elif cpu == 'hsw':
            env.Append(CCFLAGS = '-march=' + 'core-avx2')

    flags = env['CCFLAGS']
    autoflags_b = b''
    proc = subprocess.Popen(str(compiler) + ' ' + str(flags) + \
        ' -dM -E - < /dev/null', stdout=subprocess.PIPE, shell=True)
    (autoflags_b, _) = proc.communicate()

    autoflags = autoflags_b.decode('utf-8')
    if autoflags.find('__x86_64__') != -1:
        env.Append(CCFLAGS = '-D__VR_X86_64__')
    if autoflags.find('__AVX2__') != -1:
        env.Append(CCFLAGS = '-D__VR_AVX2__')
    if autoflags.find('__AVX__') != -1:
        env.Append(CCFLAGS = '-D__VR_AVX__')
    if autoflags.find('__SSE__') != -1:
        env.Append(CCFLAGS = '-D__VR_SSE__')
    if autoflags.find('__SSE2__') != -1:
        env.Append(CCFLAGS = '-D__VR_SSE2__')
    if autoflags.find('__SSE3__') != -1:
        env.Append(CCFLAGS = '-D__VR_SSE3__')
    if autoflags.find('__SSSE3__') != -1:
        env.Append(CCFLAGS = '-D__VR_SSSE3__')
    if autoflags.find('__SSE4_1__') != -1:
        env.Append(CCFLAGS = '-D__VR_SSE4_1__')
    if autoflags.find('__SSE4_2__') != -1:
        env.Append(CCFLAGS = '-D__VR_SSE4_2__')
    if autoflags.find('__AES__') != -1:
        env.Append(CCFLAGS = '-D__VR_AES__')
    if autoflags.find('__RDRND__') != -1:
        env.Append(CCFLAGS = '-D__VR_RDRND__')
    if autoflags.find('__PCLMUL__') != -1:
        env.Append(CCFLAGS = '-D__VR_PCLMUL__')

# DPDK build configuration
DPDK_TARGET = 'x86_64-native-linuxapp-gcc'
DPDK_SRC_DIR = '#third_party/dpdk/'
DPDK_DST_DIR = env['TOP'] + '/vrouter/dpdk/' + DPDK_TARGET
DPDK_INC_DIR = DPDK_DST_DIR + '/include'
DPDK_LIB_DIR = DPDK_DST_DIR + '/lib'
THRD_PRT_DIR = '#third_party/'
ADDNL_OPTION = GetOption('add-opts')

# Include paths
env.Replace(CPPPATH = '#vrouter/include')
env.Append(CPPPATH = [env['TOP'] + '/vrouter/sandesh/gen-c'])
env.Append(CPPPATH = ['#src/contrail-common'])
env.Append(CPPPATH = ['#src/contrail-common/sandesh/library/c'])

if sys.platform.startswith('win'):
    env.Append(CPPPATH = '#vrouter/windows')

# Make Sandesh quiet for production
if 'production' in env['OPT']:
    DefaultEnvironment().Append(CPPDEFINES='SANDESH_QUIET')

vr_root = './'
makefile = vr_root + 'Makefile'
dp_dir = Dir(vr_root).srcnode().abspath + '/'
make_dir = dp_dir

def MakeTestCmdFn(self, env, test_name, test_list, deps):
    sources = copy.copy(deps)
    sources.append(test_name + '.c')
    tgt = env.UnitTest(target = test_name, source = sources)
    env.Alias('vrouter:'+ test_name, tgt)
    test_list.append(tgt)
    return tgt

VRouterEnv.AddMethod(MakeTestCmdFn, 'MakeTestCmd')

def shellCommand(cmd):
    """ Return the output of a shell command
        This wrapper is required since check_output is not supported in
        python 2.6
    """
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
    (output, _) = proc.communicate()
    return output.strip()

def genBuildVersion():
    h_code = """
/*
 * Autogenerated file. Do not edit
 */
#ifndef __VR_BUILDINFO_H__
#define __VR_BUILDINFO_H__

#define VROUTER_VERSIONID "%(build)s"

#endif /* __VR_BUILDINFO_H__ */
""" % { 'build': env.GetBuildVersion()[1] }
    with open(os.path.join(dp_dir, 'include/vr_buildinfo.h'), 'w') as c_file:
        c_file.write(h_code)

if sys.platform.startswith('freebsd'):
    make_dir = make_dir + '/freebsd'
    env['ENV']['MAKEOBJDIR'] = make_dir

# XXX Temporary/transitional support for Ubuntu14.04.4 w/ kernel v4.*
#
# The logic here has to handle two different invocation models:
# default 'scons' build model; and build via packager.py build. The
# first is typical for unit-test builds.
#
# The second comes via:
# - common/debian/Makefile in contrail-packaging, which invokes:
# - debian/contrail/debian/rules.modules in contrail-packages
# This approach always uses --kernel-dir, which works for vrouter, but
# libdpdk still defaults to installed version and thus will fail.
#

if not sys.platform.startswith('win'):
    default_kernel_ver = shellCommand("uname -r").strip()
    kernel_build_dir = None
    (PLATFORM, VERSION, EXTRA) = platform.linux_distribution()
    if (PLATFORM.lower() == 'ubuntu' and VERSION.find('14.') == 0):
        if re.search('^4\.', default_kernel_ver):
            print("Warn: kernel version %s not supported for vrouter and dpdk" % default_kernel_ver)
            kernel_build_dir = '/lib/modules/3.13.0-110-generic/build'
            if os.path.isdir(kernel_build_dir):
                default_kernel_ver = "3.13.0-110-generic"
                print("info: libdpdk will be built against kernel version %s" % default_kernel_ver)
            else:
                print("*** Error: Cannot find kernel v3.13.0-110, build of vrouter will likely fail")
                kernel_build_dir = '/lib/modules/%s/build' % default_kernel_ver

    kernel_dir = GetOption('kernel-dir')
    if kernel_dir:
        kern_version = shellCommand('cat %s/include/config/kernel.release' % kernel_dir)
    else:
        kern_version = default_kernel_ver
        if kernel_build_dir: kernel_dir = kernel_build_dir
    kern_version = kern_version.strip()

if sys.platform != 'darwin':

    if not sys.platform.startswith('win'):
        install_root = GetOption('install_root')
        if install_root == None:
            install_root = ''

        src_root = install_root + '/usr/src/vrouter/'
        env.Replace(SRC_INSTALL_TARGET = src_root)
        env.Install(src_root, ['LICENSE', 'Makefile', 'GPL-2.0.txt'])
        env.Alias('install', src_root)

    buildinfo = env.GenerateBuildInfoCCode(target = ['vr_buildinfo.c'],
            source = [], path = dp_dir + 'dp-core')

    buildversion = genBuildVersion()

    exports = ['VRouterEnv']
    subdirs = [
        'include',
        'dp-core',
        'sandesh',
        'utils',
        'test',
    ]

    if platform.system() == 'Windows':
        subdirs += [
            'windows',
        ]
    else:
        subdirs += [
            'host',
            'linux',
            'uvrouter',
        ]

    if dpdk_exists and not GetOption('without-dpdk'):
        subdirs.append('dpdk')
        exports.append('dpdk_lib')
        dpdk_src_dir = Dir(DPDK_SRC_DIR).abspath
        if ADDNL_OPTION and 'enableMellanox' in ADDNL_OPTION:
            thrd_prt_dir = Dir(THRD_PRT_DIR).abspath
            mlnx_patch_cmd = 'patch -N ' + dpdk_src_dir + '/config/common_base ' + thrd_prt_dir + '/dpdk_mlnx.patch'
            os.system(mlnx_patch_cmd)

        rte_ver_filename = '../third_party/dpdk/lib/librte_eal/common/include/rte_version.h'
        rte_ver_file = open(rte_ver_filename, 'r')
        file_content = rte_ver_file.read()
        rte_ver_file.close()
        matches = re.findall("define RTE_VER_MAJOR 2", file_content)

        if matches:
            rte_libs = ('-lethdev', '-lrte_malloc')
        else:
            rte_libs = ('-lrte_ethdev',)

        year_matches = re.findall("define RTE_VER_YEAR .*", file_content)
        month_matches = re.findall("define RTE_VER_MONTH .*", file_content)
        year = int(year_matches[0].split(" ")[2])
        month = int(month_matches[0].split(" ")[2])

        if (year > 17) or (year == 17 and month >= 11):
            rte_libs = rte_libs + ('-lrte_mempool_ring', '-lrte_bus_pci', '-lrte_pci', '-lrte_bus_vdev')

        #
        # DPDK libraries need to be linked as a whole archive, otherwise some
        # callbacks and constructors will not be linked in. Also some of the
        # libraries need to be linked as a group for the cross-reference resolving.
        #
        # That is why we pass DPDK libraries as flags to the linker.
        #
        # Order is important: from higher level to lower level
        # The list is from the rte.app.mk file
        DPDK_LIBS = [
            '-Wl,--whole-archive',
        #    '-lrte_distributor',
        #    '-lrte_reorder',
            '-lrte_kni',
        #    '-lrte_ivshmem',
        #    '-lrte_pipeline',
        #    '-lrte_table',
            '-lrte_port',
            '-lrte_timer',
            '-lrte_hash',
        #    '-lrte_jobstats',
        #    '-lrte_lpm',
        #    '-lrte_power',
        #    '-lrte_acl',
        #    '-lrte_meter',
            '-lrte_sched',
            '-lm',
            '-lrt',
        #    '-lrte_vhost',
        #    '-lpcap',
        #    '-lfuse',
        #    '-libverbs',
            '-Wl,--start-group',
            '-lrte_kvargs',
            '-lrte_mbuf',
            '-lrte_ip_frag',
            rte_libs,
            '-lrte_mempool',
            '-lrte_ring',
            '-lrte_eal',
            '-lrte_cmdline',
        #    '-lrte_cfgfile',
            '-lrte_pmd_bond',
            '-lrte_pmd_bnxt',
        #    '-lrte_pmd_xenvirt',
        #    '-lxenstore',
        #    '-lrte_pmd_vmxnet3_uio',
        #    '-lrte_pmd_virtio_uio',
            '-lrte_pmd_enic',
            '-lrte_pmd_i40e',
        #    '-lrte_pmd_fm10k',
            '-lrte_pmd_ixgbe',
            '-lrte_pmd_e1000',
        #    '-lrte_pmd_mlx4',
        #    '-lrte_pmd_ring',
        #    '-lrte_pmd_pcap',
            '-lrte_pmd_af_packet'
        ]
        if ADDNL_OPTION and 'enableMellanox' in ADDNL_OPTION:
            DPDK_LIBS.append('-lrte_pmd_mlx5')
            DPDK_LIBS.append('-libverbs')
            DPDK_LIBS.append('-lmlx5')

        DPDK_LIBS.append('-Wl,--end-group')
        DPDK_LIBS.append('-Wl,--no-whole-archive')

        if year_matches and month_matches:
            DPDK_LIBS.append('-Wl,-lnuma')

        # Pass -g and -O flags if present to DPDK
        DPDK_FLAGS = ' '.join(o for o in env['CCFLAGS'] if ('-g' in o or '-O' in o))

        # Make DPDK
        dpdk_dst_dir = Dir(DPDK_DST_DIR).abspath

        make_cmd = 'make -C ' + dpdk_src_dir \
            + ' EXTRA_CFLAGS="' + DPDK_FLAGS \
            + ' -Wno-maybe-uninitialized' \
            + '"' \
            + ' ARCH=x86_64' \
            + ' O=' + dpdk_dst_dir \
            + ' '

        # If this var is set, then we need to pass it to make cmd for libdpdk
        if kernel_build_dir:
            print("info: Adjusting libdpdk build to use RTE_KERNELDIR=%s" % kernel_build_dir)
            make_cmd += "RTE_KERNELDIR=%s " % kernel_build_dir

        dpdk_lib = env.Command('dpdk_lib', None,
            make_cmd + 'config T=' + DPDK_TARGET
            + ' && ' + make_cmd)

        env.Append(CPPPATH = DPDK_INC_DIR);
        env.Append(LIBPATH = DPDK_LIB_DIR)
        env.Append(DPDK_LINKFLAGS = DPDK_LIBS)

        if GetOption('clean'):
            os.system(make_cmd + 'clean')

    for sdir in subdirs:
        env.SConscript(sdir + '/SConscript',
                       exports = exports,
                       variant_dir = env['TOP'] + '/vrouter/' + sdir,
                       duplicate = 0)

    if sys.platform.startswith('win'):
        def make_cmd(target, source, env):
            msbuild = [
                os.environ['MSBUILD'],
                'windows/vRouter.sln',
                '/p:Platform=x64',
                '/p:Configuration=' + env['VS_BUILDMODE']
            ]

            subprocess.call(msbuild, cwd=Dir('#/vrouter').abspath)
        vrouter_target = File('#/build/' + env['OPT'] + '/vrouter/extension/vRouter/vRouter.sys')
    else:
        make_cmd = 'cd ' + make_dir + ' && make'
        if kernel_dir: make_cmd += ' KERNELDIR=' + kernel_dir
        make_cmd += ' SANDESH_HEADER_PATH=' + Dir(env['TOP'] + '/vrouter/').abspath
        make_cmd += ' SANDESH_SRC_ROOT=' + '../build/kbuild/'
        make_cmd += ' SANDESH_EXTRA_HEADER_PATH=' + Dir('#src/contrail-common/').abspath
        vrouter_target = 'vrouter.ko'
    if 'vrouter' in COMMAND_LINE_TARGETS:
        if not sys.platform.startswith('win'):
            BUILD_TARGETS.append('vrouter/uvrouter')
        else:
            BUILD_TARGETS.append('vrouter.msi')
        if dpdk_exists:
            BUILD_TARGETS.append('vrouter/dpdk')
        BUILD_TARGETS.append('vrouter/utils')

    kern = env.Command(vrouter_target, None, make_cmd)
    env.Default(kern)
    env.AlwaysBuild(kern)

    env.Requires(kern, ['#build/lib/cmocka.lib', '#build/include/cmocka.h'])

    env.Depends(kern, buildinfo)
    env.Depends(kern, buildversion)
    env.Depends(kern, env.Install(
                '#build/kbuild/sandesh/gen-c',
                env['TOP'] + '/vrouter/sandesh/gen-c/vr_types.c'))
    sandesh_lib = [
        'protocol/thrift_binary_protocol.c',
        'protocol/thrift_protocol.c',
        'sandesh.c',
        'transport/thrift_fake_transport.c',
        'transport/thrift_memory_buffer.c',
        ]
    for src in sandesh_lib:
        dirname = os.path.dirname(src)
        env.Depends(kern,
                env.Install(
                    '#build/kbuild/sandesh/library/c/' + dirname,
                    env['TOP'] + '/tools/sandesh/library/c/' + src))

    if not sys.platform.startswith('win'):
        if GetOption('clean') and (not COMMAND_LINE_TARGETS or 'vrouter' in COMMAND_LINE_TARGETS):
            os.system(make_cmd + ' clean')

        libmod_dir = install_root
        libmod_dir += '/lib/modules/%s/extra/net/vrouter' % kern_version
        env.Alias('build-kmodule', env.Install(libmod_dir, kern))
    else:
        env.Append(WIXCANDLEFLAGS = ['-doptimization=' + env['OPT']])
        env.Append(WIXLIGHTFLAGS = ['-ext', 'WixUtilExtension.dll'])
        msi_command = env.WiX(File('#/build/' + env['OPT'] + '/vrouter/extension/vRouter.msi'), ['windows/installer/vrouter_msi.wxs'])
        env.Depends(msi_command, kern)
        env.Alias('vrouter.msi', msi_command)

# Local Variables:
# mode: python
# End:
