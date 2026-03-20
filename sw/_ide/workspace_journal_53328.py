# 2026-03-18T15:30:08.962939800
import vitis

client = vitis.create_client()
client.set_workspace(path="sw")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="lwip_udp_perf_server")
comp.build()

vitis.dispose()

