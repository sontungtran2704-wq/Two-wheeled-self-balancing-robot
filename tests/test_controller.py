import sys,unittest,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from control_reference import Controller

class MixerTests(unittest.TestCase):
    def test_balance_priority_respects_each_wheel_acceleration_at_speed_limit(self):
        c=Controller();c.q=.58;c.d=.02
        before=[c.q-c.d,c.q+c.d]
        z=c.tick(.2,0,.58,0,0,0,False,False)
        for old,new in zip(before,[z['left'],z['right']]):
            self.assertLessEqual(abs(new-old)/c.dt,3+1e-9)
            self.assertLessEqual(abs(new),.6+1e-9)

    def test_turn_reversal_limited(self):
        c=Controller();previous=[0.,0.]
        for n in range(2000):
            z=c.tick(.01*math.sin(n/30),0,c.q,2*c.d/.228,.2,.8 if n<1000 else -.8,True,True)
            for old,new in zip(previous,[z['left'],z['right']]):
                self.assertLessEqual(abs(new-old)/c.dt,3+1e-8)
                self.assertLessEqual(abs(new),.6+1e-9)
            previous=[z['left'],z['right']]

if __name__=='__main__':unittest.main()
