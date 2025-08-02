#include "udf.h"
#include "math.h"

/* 参数 */
#define BATTERY_CAPACITY 3.2        /* Ah */
#define NOMINAL_VOLTAGE 3.6         /* V */

/* 工况 */
#define C_RATE 2.0                  /* C倍率*/

/* 论文提到但未给出具体数值的参数,供参考 */
#define INTERNAL_RESISTANCE 0.045   /* Ω - 内阻 */
#define ENTROPY_COEFF -0.4e-3      /* V/K -电池典型熵系数 */


#define TIME_CONSTANT 3600.0        /* s - τ扩散时间常数 */
#define OCV_SOC_DERIVATIVE 1.0      /* V - ∂UOCV,therm/∂S */

#define BATTERY_RADIUS 0.009       /* m - 半径*/
#define BATTERY_HEIGHT 0.065       /* m - 高度 */
#define TOTAL_BATTERY_VOLUME (M_PI * BATTERY_RADIUS * BATTERY_RADIUS * BATTERY_HEIGHT)  /* ≈1.65e-5 m³ */

/* 电池电流*/
real get_battery_current(void)
{
    return C_RATE * BATTERY_CAPACITY;  /* A */
}

/* 活化过电位 */
real calculate_activation_overpotential(real Ib, real temp)
{
    real F = 96485.0;  
    real R = 8.314;     
 
    real J0 = 0.5;     
    real I1C = BATTERY_CAPACITY;  

    real argument = Ib / (2.0 * J0 * I1C);
    

    real eta_act = 2.0 * R * temp / F * asinh(argument);
    
    
    if (eta_act > 0.5) eta_act = 0.5;
    if (eta_act < 0.0) eta_act = 0.0;
    
    return eta_act;  /* V */
}

/* 混合项 */
real calculate_qmix_power(real Ib, real time, real temp)
{
    real qmix_power = 0.0;  /* W */
    
    if (time > 0.0) {
        
        real dQcell = BATTERY_CAPACITY * 3600.0;  
        real dQcell_tau = dQcell / TIME_CONSTANT;
        
      
        real soc_rate = Ib / dQcell; 
        real time_factor = 1.0 - exp(-time / TIME_CONSTANT);
        
   
        real gradient_integral = soc_rate * time_factor / 3.0;  /* 1/s */
        
       
        qmix_power = dQcell_tau * OCV_SOC_DERIVATIVE * gradient_integral;  /* A * V * 1/s = W */
        
   
        real temp_factor = exp(-1500.0 * (1.0/temp - 1.0/298.15));
        qmix_power *= temp_factor;
    }
    
    return qmix_power;  /* W */
}

DEFINE_SOURCE(battery_heat_source, cell, thread, dS, eqn)
{
    real temp = C_T(cell, thread);       /* K */
    real time = CURRENT_TIME;            /* s */
    
    /* 电池电流 */
    real Ib = get_battery_current();     /* A */

    /* 1. 欧姆损失 */
    real power_ohmic = Ib * Ib * INTERNAL_RESISTANCE;  /* A² × Ω = W ✓ */
    
    /* 2. 活化过电位*/
    real eta_act = calculate_activation_overpotential(Ib, temp);  /* V */
    real power_activation = Ib * eta_act;  /* A × V = W ✓ */
    
    /* 3. 熵变 */
    real power_entropic = Ib * temp * ENTROPY_COEFF;  
    
    /* 4. 混合项*/
    real power_mix = calculate_qmix_power(Ib, time, temp);  
    
    /* 总功率*/
    real total_power = power_ohmic + power_activation + power_entropic + power_mix; 
   
    real source_density = total_power / TOTAL_BATTERY_VOLUME;  
 
    if (source_density < 0.0) source_density = 0.0;
 
    dS[eqn] = (Ib * ENTROPY_COEFF) / TOTAL_BATTERY_VOLUME;  
    

    return source_density;  
}
