import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from design import load_config, coefficients, dynamics, design_lqr
import numpy as np

class ModelTest(unittest.TestCase):
    def test_linearization_and_motor_reaction(self):
        p=load_config()['model']; M,J,H,b=coefficients(p)
        A,B,*_=design_lqr(load_config())
        eps=1e-6
        np.testing.assert_allclose(dynamics([eps,0,0,0],0,p)[1]/eps,A[1,0],rtol=1e-9)
        np.testing.assert_allclose(dynamics([0,0,0,0],eps,p)[1]/eps,B[1,0],rtol=1e-9)
        self.assertLess(B[1,0],0)
        self.assertGreater(b, p['mb']*p['l'])
    def test_closed_loop(self):
        _,_,Ad,Bd,K,_=design_lqr(load_config())
        self.assertLess(max(abs(np.linalg.eigvals(Ad-Bd@K))),1)
        self.assertGreater((-K@np.array([0.05,0])).item(),0)

if __name__=='__main__': unittest.main()
