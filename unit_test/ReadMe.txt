TODOs

- detect disconnect with timeout (use Connection::m_recvTime)
- cleanup send buffer on timer, use one timer in socket
- lazy remove dead connections

to resume, create timer in socket to do all housekeeping work

- add more connection statistics, total # of sent packets, acked, lost, data bandwidth
- calc average buffers load (update from house-keeping timer)

- design heartbit protocol

- big task: create protocol for reliable object (file or big buffer) delivery

