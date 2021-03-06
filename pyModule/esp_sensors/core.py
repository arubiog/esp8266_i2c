import socket
import functools
import time

class TemperatureSensors(object):


    def __init__(self, host, port=23, timeout=3):
        self.rs = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.rs.settimeout(timeout)
        self.rs.connect((host, port))

        # Creating dynamically properties for each sensor
        self.nr_sensors = int(self._sendCmd('sensors').split(':')[1])

        for sensor_nr in range(1, self.nr_sensors+1):
            temp_name = 'T%d' % sensor_nr
            f_temp = property(functools.partial(TemperatureSensors.read_temp,
                                                sensor_nr=sensor_nr))
            setattr(TemperatureSensors, temp_name, f_temp)


    def __delete__(self):
        self.rs.close()

    def _sendCmd(self, cmd, size=8096):
        while len(cmd):
            n = self.rs.send(cmd)
            cmd = cmd[n:]
        while True:
            answer = self.rs.recv(size)
            if answer[-1] == '\n':
                break
        return answer[:-1]

    def read_hardware(self):
        self.raw_temps = self._sendCmd('temp').split(';')[:-1]
        self.raw_states = self._sendCmd('state')[:-1]

    @property
    def state(self):
        if 'OK' in self.raw_states.upper():
            state = 'OK'
            status = 'All sensors are in ranges'
        else:
            state = 'ALARM'
            status = self.raw_states
        return state, status

    def read_temp(self, sensor_nr):
        self.read_hardware()
        start = len('T%d:' % sensor_nr)
        temp = self.raw_temps[sensor_nr - 1][start:]
        return temp



def basicTest(host, port=23):
    rs = TemperatureSensors(host, port)
    print 'NrSensors: ', rs.nr_sensors
    for i in range(1, rs.nr_sensors+1):
        t_name = 'T%d' % i
        print '%s:%r' % (t_name, getattr(rs, t_name))
    print 'Device State: %r %r' % rs.state

if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1:
        host = sys.argv[1]
    else:
        host = '10.210.1.135'
    basicTest(host)

