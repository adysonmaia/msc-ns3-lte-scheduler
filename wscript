## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):

    module = bld.create_ns3_module('lte', ['core', 'network', 'spectrum', 'stats', 'buildings', 'virtual-net-device','point-to-point','applications','internet','csma', 'lte'])
    module.source = [
        'm2m-mac-scheduler.cc',
        ]

    module_test = bld.create_ns3_module_test_library('lte-m2m')
    module_test.source = [
        ]

    headers = bld(features='ns3header')
    headers.module = 'lte'
    headers.source = [
        'm2m-mac-scheduler.h',
        ]

    bld.ns3_python_bindings()
