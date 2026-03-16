# 2026-03-16T14:49:53.402018100
import vitis

client = vitis.create_client()
client.set_workspace(path="sw")

platform = client.create_platform_component(name = "platform",hw_design = "$COMPONENT_LOCATION/../../../../../usb_uart_test/design_1_wrapper.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",compiler = "gcc")

platform = client.get_component(name="platform")
domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_lib(lib_name="lwip220", path="C:\AMDDesignTools\2025.2\Vitis\data\embeddedsw\ThirdParty\sw_services\lwip220_v1_3")

status = domain.set_config(option = "lib", param = "XILTIMER_en_interval_timer", value = "true", lib_name="xiltimer")

status = domain.set_config(option = "lib", param = "lwip220_dhcp", value = "true", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_lwip_dhcp_does_acd_check", value = "true", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_memp_n_pbuf", value = "1024", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_mem_size", value = "524288", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_pbuf_pool_size", value = "16384", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_n_rx_descriptors", value = "512", lib_name="lwip220")

status = platform.build()

comp = client.create_app_component(name="app_component",platform = "$COMPONENT_LOCATION/../platform/export/platform/platform.xpfm",domain = "standalone_ps7_cortexa9_0")

status = platform.build()

comp = client.get_component(name="app_component")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

status = platform.build()

comp.build()

component = client.get_component(name="app_component")

lscript = component.get_ld_script(path="C:\Users\nicol\Documents\GitHub\proyecto-modulacion-fpga\sw\app_component\src\lscript.ld")

lscript.regenerate()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

status = domain.regenerate()

status = platform.build()

status = comp.clean()

comp = client.get_component(name="app_component")
comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.h", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.h", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/main.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.h"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.h", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/main.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.h"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.h", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.h"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.h"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.c"])

comp.set_app_config(key = "USER_HEADER_SOURCES", values = ["C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.h", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.h", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.h"])

status = platform.build()

comp = client.get_component(name="app_component")
comp.build()

comp = client.get_component(name="app_component")
comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["src/sfp.c", "src/si5324.c", "src/i2c_access.c", "src/platform.c", "src/iic_phyreset.c", "src/udp_perf_server.c"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["src/si5324.c", "src/i2c_access.c", "src/platform.c", "src/iic_phyreset.c", "src/udp_perf_server.c"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["src/i2c_access.c", "src/platform.c", "src/iic_phyreset.c", "src/udp_perf_server.c"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["src/platform.c", "src/iic_phyreset.c", "src/udp_perf_server.c"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["src/iic_phyreset.c", "src/udp_perf_server.c"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["src/udp_perf_server.c"])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = [""])

comp.set_app_config(key = "USER_COMPILE_SOURCES", values = ["C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/main.c", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.c"])

comp.set_app_config(key = "USER_HEADER_SOURCES", values = ["C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/dma.h", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/interrupts.h", "C:/Users/nicol/Documents/GitHub/proyecto-modulacion-fpga/sw/app_component/src/network.h"])

comp.set_app_config(key = "USER_INCLUDE_DIRECTORIES", values = ["src"])

comp = client.get_component(name="app_component")
status = comp.clean()

status = platform.build()

comp.build()

status = comp.clean()

vitis.dispose()

