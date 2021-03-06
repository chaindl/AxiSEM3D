// Mass3D.cpp
// created by Kuangdai on 2-Jun-2016 
// 3D mass

#include "ABC.h"
#include "SolverFFTW_3.h"
#include "SolverFFTW_1.h"

ABC::ABC(const RColX gamma, const RColX v_rho_p, const RColX v_rho_s, const int nr): 
mGamma(gamma), mVp_Rho(v_rho_p), mVs_Rho(v_rho_s), mNr(nr) {
    mKosloff = (mGamma.real().sum() > tinyDouble);
    mNu = mNr / 2;
    
    mKosloff3D = (mGamma.maxCoeff() - mGamma.minCoeff() > tinyDouble);
    mStacey3D = (mVp_Rho.rows() > 1);
}

void ABC::setNormal(RMatX3 const normal) {
    mWeight = normal.row(0).norm();
    mNormal = normal.array() / mWeight;
    mStacey = mWeight > tinyDouble;
}

CMatX3 ABC::StaceyTraction(const CMatX3 veloc) const {
    if (!mStacey) {
        return CMatX3::Zero(mNu + 1, 3);
    }
    
    CMatX3 tractionC;
    if (mStacey3D) {
        CMatX3 &velocC = SolverFFTW_3::getC2R_CMat();
        velocC.topRows(mNu + 1) = veloc;
        
        // FFT forward    
        SolverFFTW_3::computeC2R(mNr);
        RMatX3 velocR = SolverFFTW_3::getC2R_RMat();
        RMatX3 &tractionR = SolverFFTW_3::getR2C_RMat();
        for (int alpha = 0; alpha < mNr; alpha++) {
            RRow3 vn = (velocR.row(alpha) * mNormal.row(alpha).transpose())(0,0) * mNormal.row(alpha);
            RRow3 vt = velocR.row(alpha) - vn;
            tractionR.row(alpha) = (mVp_Rho(alpha) * vn + mVs_Rho(alpha) * vt) * mWeight;
            
        }
        // FFT backward
        SolverFFTW_3::computeR2C(mNr);
        tractionC = SolverFFTW_3::getR2C_CMat().topRows(mNu + 1);
     } else {
        tractionC = CMatX3::Zero(mNu + 1, 3);
        for (int alpha = 0; alpha <= mNu; alpha++) {
            CRow3 vn = (veloc.row(alpha) * mNormal.row(0).transpose())(0,0) * mNormal.row(0);
            CRow3 vt = veloc.row(alpha) - vn;
            tractionC.row(alpha) = (mVp_Rho(0) * vn + mVs_Rho(0) * vt) * mWeight;
         }
     }
    
    return tractionC;
}

CColX ABC::StaceyTraction(const CColX veloc) const {
    if (!mStacey) {
        return CColX::Zero(mNu + 1, 1);
    }
    
    CColX tractionC;
    if (mStacey3D) {
        CColX &velocC = SolverFFTW_1::getC2R_CMat();
        velocC.topRows(mNu + 1) = veloc;
        
        // FFT forward    
        SolverFFTW_1::computeC2R(mNr);
        RColX velocR = SolverFFTW_1::getC2R_RMat().topRows(mNr);
        RColX &tractionR = SolverFFTW_1::getR2C_RMat();

        tractionR.topRows(mNr) = mVp_Rho.array().pow(-1.) * velocR.array() * mWeight;

        // FFT backward
        SolverFFTW_1::computeR2C(mNr);
        tractionC = SolverFFTW_1::getR2C_CMat().topRows(mNu + 1);
    } else {
        tractionC = veloc / mVp_Rho(0) * mWeight;
    }
    
    return tractionC;
}

void ABC::applyKosloffDamping(CMatX3 &accel, const CMatX3 veloc, const CMatX3 displ) {
    if (!mKosloff) {
        return;
    }
    
    if (mKosloff3D) {
        CMatX3 &displC = SolverFFTW_3::getC2R_CMat();
        displC.topRows(mNu + 1) = displ;
        SolverFFTW_3::computeC2R(mNr);
        RMatX3 displR = SolverFFTW_3::getC2R_RMat();
        
        CMatX3 &velocC = SolverFFTW_3::getC2R_CMat();
        velocC.topRows(mNu + 1) = veloc;
        SolverFFTW_3::computeC2R(mNr);
        RMatX3 velocR = SolverFFTW_3::getC2R_RMat();
        
        RMatX3 &accelR = SolverFFTW_3::getR2C_RMat();
        
        CMatX3 &accelC = SolverFFTW_3::getC2R_CMat();
        accelC.topRows(mNu + 1) = accel;
        SolverFFTW_3::computeC2R(mNr);
        accelR = SolverFFTW_3::getC2R_RMat();

        for (int alpha = 0; alpha < mNr; alpha++) {
            accelR.row(alpha) -= mGamma(alpha) * velocR.row(alpha) + mGamma(alpha) * velocR.row(alpha) + mGamma(alpha) * mGamma(alpha) * displR.row(alpha);
        }
        
        SolverFFTW_3::computeR2C(mNr);
        accel = SolverFFTW_3::getR2C_CMat().topRows(mNu + 1);
        
    } else {
        
        for (int alpha = 0; alpha <= mNu; alpha++) {
            accel.row(alpha) -= mGamma(0) * veloc.row(alpha) + mGamma(0) * veloc.row(alpha) + mGamma(0) * mGamma(0) * displ.row(alpha);
        }
    }
}

void ABC::applyKosloffDamping(CColX &accel, const CColX veloc, const CColX displ) {
    if (!mKosloff) {
        return;
    }
    
    if (mKosloff3D) {
        CColX &displC = SolverFFTW_1::getC2R_CMat();
        displC.topRows(mNu + 1) = displ;
        SolverFFTW_1::computeC2R(mNr);
        RColX displR = SolverFFTW_1::getC2R_RMat();
        
        CColX &velocC = SolverFFTW_1::getC2R_CMat();
        velocC.topRows(mNu + 1) = veloc;
        SolverFFTW_1::computeC2R(mNr);
        RColX velocR = SolverFFTW_1::getC2R_RMat();
        
        RColX &accelR = SolverFFTW_1::getR2C_RMat();
        
        CColX &accelC = SolverFFTW_1::getC2R_CMat();
        accelC.topRows(mNu + 1) = accel;
        SolverFFTW_1::computeC2R(mNr);
        accelR = SolverFFTW_1::getC2R_RMat();

        for (int alpha = 0; alpha < mNr; alpha++) {
            accelR(alpha) -=  mGamma(alpha) * velocR(alpha) + mGamma(alpha) * velocR(alpha) + mGamma(alpha) * mGamma(alpha) * displR(alpha);
        }
        
        SolverFFTW_1::computeR2C(mNr);
        accel = SolverFFTW_1::getR2C_CMat().topRows(mNu + 1);
        
    } else {
        for (int alpha = 0; alpha <= mNu; alpha++) {
            accel(alpha) -=  mGamma(0) * veloc(alpha) + mGamma(0) * veloc(alpha) + mGamma(0) * mGamma(0) * displ(alpha);
        }
    }

}