# 2026-03-23T15:16:16.137277900
import vitis

client = vitis.create_client()
client.set_workspace(path="sw")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="lwip_udp_perf_server")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_config(option = "lib", param = "lwip220_n_rx_descriptors", value = "512", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_n_tx_descriptors", value = "512", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_pbuf_pool_size", value = "3072", lib_name="lwip220")

status = domain.set_config(option = "lib", param = "lwip220_ip_reass_max_pbufs", value = "256", lib_name="lwip220")

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

