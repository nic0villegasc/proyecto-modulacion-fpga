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

