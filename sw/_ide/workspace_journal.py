# 2026-03-16T14:49:53.402018100
import vitis

client = vitis.create_client()
client.set_workspace(path="sw")

platform = client.create_platform_component(name = "platform",hw_design = "$COMPONENT_LOCATION/../../../../../usb_uart_test/design_1_wrapper.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",compiler = "gcc")

platform = client.get_component(name="platform")
domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_lib(lib_name="lwip220", path="C:\AMDDesignTools\2025.2\Vitis\data\embeddedsw\ThirdParty\sw_services\lwip220_v1_3")

