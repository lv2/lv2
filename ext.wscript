#!/usr/bin/env python
import glob
import os
import shutil

from waflib.extras import autowaf as autowaf

# A rule for making a link in the build directory to a source file
def link(task):
    if hasattr(os, 'symlink'):
        func = os.symlink
    else:
        func = shutil.copy  # Symlinks unavailable, make a copy

    try:
        os.remove(task.outputs[0].abspath())  # Remove old target
    except:
        pass  # No old target, whatever

    func(task.inputs[0].abspath(), task.outputs[0].abspath())

def configure(conf):
    pass

def options(opt):
    opt.load('compiler_c')
    autowaf.set_options(opt)
    opt.add_option('--test', action='store_true', default=False, dest='build_tests',
                   help="Build unit tests")
    opt.add_option('--copy-headers', action='store_true', default=False,
                   dest='copy_headers',
                   help='Copy headers instead of linking to bundle')

def build(bld):
    path        = bld.path.srcpath()[len('lv2/'):]
    name        = os.path.basename(path)
    bundle_dir  = os.path.join(bld.env['LV2DIR'], name + '.lv2')
    include_dir = os.path.join(bld.env['INCLUDEDIR'], 'lv2', path)

    # Copy headers to URI-style include paths in build directory
    for i in bld.path.ant_glob('*.h'):
        bld(rule   = link,
            name   = 'link',
            cwd    = 'build/lv2/%s' % path,
            source = '%s' % i,
            target = 'lv2/%s/%s' % (path, i))

    if bld.env['BUILD_TESTS'] and bld.path.find_node('%s-test.c' % name):
        test_lib    = []
        test_cflags = ['']
        if bld.is_defined('HAVE_GCOV'):
            test_lib    += ['gcov']
            test_cflags += ['-fprofile-arcs', '-ftest-coverage']

        # Unit test program
        bld(features     = 'c cprogram',
            source       = '%s-test.c' % name,
            lib          = test_lib,
            target       = '%s-test' % name,
            install_path = '',
            cflags       = test_cflags)
            
    # Install bundle
    bld.install_files(bundle_dir,
                      bld.path.ant_glob('?*.*', excl='*.in'))

    # Install URI-like includes
    if bld.path.ant_glob('*.h'):
        if bld.env['COPY_HEADERS']:
            bld.install_files(include_dir, bld.path.ant_glob('*.h'))
        else:
            bld.symlink_as(include_dir,
                           os.path.relpath(bundle_dir,
                                           os.path.dirname(include_dir)))

def test(ctx):
    name = os.path.basename(ctx.path.srcpath()[len('lv2/'):])
    autowaf.pre_test(ctx, name, dirs=['.'])
    os.environ['PATH'] = '.' + os.pathsep + os.getenv('PATH')
    autowaf.run_tests(ctx, name, ['%s-test' % name], dirs=['.'])
    autowaf.post_test(ctx, name, dirs=['.'])

def news(ctx):
    path = ctx.path.abspath()
    autowaf.write_news(os.path.basename(path),
                       glob.glob(os.path.join(path, '*.ttl')),
                       os.path.join(path, 'NEWS'))

def pre_dist(ctx):
    # Write NEWS file in source directory
    news(ctx)

def post_dist(ctx):
    # Delete generated NEWS file from source directory
    try:
        os.remove(os.path.join(ctx.path.abspath(), 'NEWS'))
    except:
        pass
