opts = Options()
opts.Add('DESTDIR', 'Directory to install', '/')
opts.Add('DEBUG', 'Set DEBUG=yes for building with enabled debuging', 'no')
opts.Add('NLS_SUPPORT', 'Set NLS_SUPPORT=no for building with disable gettext i18n', 'yes')
opts.Add('LIBDIR', 'Set LIBDIR=/lib/dir/path for change default instalation path', '/usr/lib')
opts.Add('LIBSYSDIR', 'Set LIBSYSDIR=/lib/system/dir/path for change default instalation path', '/lib')
env = Environment(options=opts)
Help(opts.GenerateHelpText(env))

LIBDEVSUFFIX = ".so"

NSS_NAME = "libnss_role"
NSS_LIBBASEVERSION = "2"
NSS_LIBSUBVERSION = "0.0"
NSS_LIBFULLVERSION = NSS_LIBBASEVERSION + '.' + NSS_LIBSUBVERSION
NSS_LIBBASESUFFIX = LIBDEVSUFFIX + '.' + NSS_LIBBASEVERSION
NSS_LIBFULLSUFFIX = LIBDEVSUFFIX + '.' + NSS_LIBFULLVERSION
NSS_SONAME = NSS_NAME + NSS_LIBBASESUFFIX
NSS_FULLNAME = NSS_NAME + NSS_LIBFULLSUFFIX

COMMON_NAME = "librole"
COMMON_LIBBASEVERSION = "1"
COMMON_LIBSUBVERSION = "0.0"
COMMON_LIBFULLVERSION = COMMON_LIBBASEVERSION + '.' + COMMON_LIBSUBVERSION
COMMON_LIBBASESUFFIX = LIBDEVSUFFIX + '.' + COMMON_LIBBASEVERSION
COMMON_LIBFULLSUFFIX = LIBDEVSUFFIX + '.' + COMMON_LIBFULLVERSION
COMMON_SONAME = COMMON_NAME + COMMON_LIBBASESUFFIX
COMMON_FULLNAME = COMMON_NAME + COMMON_LIBFULLSUFFIX
COMMON_DEVNAME = COMMON_NAME + LIBDEVSUFFIX

env['CCFLAGS'] = ['-Iinclude', '-DPACKAGE=\\"nss_role\\"']
if 'DEBUG' in ARGUMENTS and ARGUMENTS['DEBUG'] == 'yes':
    env['CCFLAGS'] += ['-DDEBUG', '-g', '-O0']
else:
    env['CCFLAGS'] += ['-g', '-O2']
if 'NLS_SUPPORT' not in ARGUMENTS or ARGUMENTS['NLS_SUPPORT'] != 'no':
    env['CCFLAGS'] += ['-DENABLE_NLS', '-DLOCALEDIR=\\"/usr/share/locale\\"']

# TODO: link libnss-role with librole
# common modules, skip linking libnss-role with librole
parser = env.SharedObject('parser', 'parser.c')
locking = env.SharedObject('locking', 'lock_file.c')
pamcheck = env.SharedObject('pamcheck', 'pam_check.c')
common = env.SharedObject('common', 'common.c')

libenv = env.Clone()
libenv["SHLIBSUFFIX"] = [NSS_LIBFULLSUFFIX]
libenv["LIBS"] = ['pam', 'pam_misc']
libenv["LINKFLAGS"] = ['-Wl,-soname,' + NSS_SONAME]
so = libenv.SharedLibrary(NSS_NAME, ['nss_role.c', common, parser, locking, pamcheck, 'src/fileop.c'])
solink = libenv.Command(NSS_SONAME, so[0], 'ln -sf %s %s' % (NSS_FULLNAME, NSS_SONAME))

commonenv = env.Clone()
commonenv["SHLIBSUFFIX"] = [COMMON_LIBFULLSUFFIX]
commonenv["LIBS"] = ['pam', 'pam_misc']
commonenv["LINKFLAGS"] = ['-Wl,-soname,' + COMMON_SONAME]
common = commonenv.SharedLibrary(COMMON_NAME, [common, parser, locking, pamcheck, 'graph.c', 'src/fileop.c'])
commonlink = commonenv.Command(COMMON_SONAME, common[0], 'ln -sf %s %s' % (COMMON_FULLNAME, COMMON_SONAME))
commondevlink = commonenv.Command(COMMON_DEVNAME, common[0], 'ln -sf %s %s' % (COMMON_FULLNAME, COMMON_DEVNAME))
commonheaders = Glob('include/role/*.h')

utilenv = env.Clone()
utilenv["LIBS"] = ['role']
utilenv["LIBPATH"] = '.'

roleadd = utilenv.Program('roleadd', 'roleadd.c')
roledel = utilenv.Program('roledel', 'roledel.c')
rolelst = utilenv.Program('rolelst', ['rolelst.c', 'src/fileop.c'])
test_role = utilenv.Program('test_role', 'test_role.c')

commonenv.Install('$DESTDIR/$LIBDIR/', common)
commonenv.Install('$DESTDIR/usr/include/role', commonheaders)
i = commonenv.Command('$DESTDIR/$LIBDIR/' + COMMON_SONAME, commonlink[0], 'cp -P %s /$DESTDIR/$LIBDIR/%s' % (COMMON_SONAME, COMMON_SONAME))
commonenv.Alias('install', i)
i = commonenv.Command('$DESTDIR/$LIBDIR/' + COMMON_DEVNAME, commondevlink[0], 'cp -P %s /$DESTDIR/$LIBDIR/%s' % (COMMON_DEVNAME, COMMON_DEVNAME))
commonenv.Alias('install', i)

utilenv.Install('$DESTDIR/usr/sbin', [roleadd, roledel])
utilenv.Install('$DESTDIR/usr/bin', [rolelst])
utilenv.InstallAs('$DESTDIR/etc/pam.d/roleadd', 'role.pamd')
utilenv.InstallAs('$DESTDIR/etc/pam.d/roledel', 'role.pamd')
utilenv.Install('$DESTDIR/usr/share/man/man8', ['roleadd.8', 'roledel.8', 'rolelst.8'])

libenv.Install('$DESTDIR/$LIBSYSDIR', so)
i = libenv.Command('$DESTDIR/$LIBSYSDIR/' + NSS_SONAME, solink[0], 'cp -P %s /$DESTDIR/$LIBSYSDIR/%s' % (NSS_SONAME, NSS_SONAME))
libenv.Alias('install', i)

Alias('install', FindInstalledFiles())
