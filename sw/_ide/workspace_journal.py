# 2026-03-17T11:10:30.940759900
import vitis

client = vitis.create_client()
client.set_workspace(path="sw")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="lwip_udp_perf_server")
comp.build()

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_config(option = "lib", param = "lwip220_pbuf_pool_size", value = "4096", lib_name="lwip220")

status = domain.update_path(option = "OS",name="standalone", new_path = "C:/AMDDesignTools/2025.2/Vitis/data/embeddedsw/lib/bsp/standalone_v9_4")

status = domain.update_path(option = "OS",name="standalone", new_path = "C:/AMDDesignTools/2025.2/Vitis/data/embeddedsw/lib/bsp/standalone_v9_4")

status = platform.build()

status = domain.set_config(option = "lib", param = "lwip220_n_rx_descriptors", value = "1024", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_n_tx_descriptors", value = "1024", lib_name="lwip220")

status = platform.build()

status = platform.build()

comp.build()

status = domain.set_config(option = "lib", param = "lwip220_n_rx_descriptors", value = "512", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_n_tx_descriptors", value = "512", lib_name="lwip220")

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = domain.set_config(option = "lib", param = "lwip220_n_rx_descriptors", value = "1024", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_n_tx_descriptors", value = "1024", lib_name="lwip220")

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

