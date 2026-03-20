# 2026-03-20T11:31:58.517021
import vitis

client = vitis.create_client()
client.set_workspace(path="sw")

platform = client.get_component(name="platform")
status = platform.build()

domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_config(option = "lib", param = "lwip220_n_rx_descriptors", value = "128", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_n_tx_descriptors", value = "128", lib_name="lwip220")

status = platform.build()

status = domain.set_config(option = "lib", param = "lwip220_pbuf_pool_size", value = "2500", lib_name="lwip220")

status = platform.build()

status = platform.build()

status = platform.build()

comp = client.get_component(name="lwip_udp_perf_server")
comp.build()

status = platform.build()

comp.build()

