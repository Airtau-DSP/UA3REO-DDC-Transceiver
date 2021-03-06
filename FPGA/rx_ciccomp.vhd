-- -------------------------------------------------------------
--
-- Module: rx_ciccomp
-- Generated by MATLAB(R) 9.4 and Filter Design HDL Coder 3.1.3.
-- Generated on: 2019-04-07 11:59:39
-- -------------------------------------------------------------

-- -------------------------------------------------------------
-- HDL Code Generation Options:
--
-- TargetLanguage: VHDL
-- OptimizeForHDL: on
-- EDAScriptGeneration: off
-- Name: rx_ciccomp
-- SerialPartition: 16
-- TestBenchName: rx_ciccomp_tb
-- TestBenchStimulus: step ramp chirp noise 
-- GenerateHDLTestBench: off

-- Filter Specifications:
--
-- Sample Rate            : N/A (normalized frequency)
-- Response               : CIC Compensator
-- Specification          : N,Fp,Fst
-- Decimation Factor      : 2
-- Multirate Type         : Decimator
-- CIC Rate Change Factor : 512
-- Stopband Edge          : 0.55
-- Passband Edge          : 0.45
-- Filter Order           : 64
-- Number of Sections     : 5
-- Differential Delay     : 1
-- -------------------------------------------------------------

-- -------------------------------------------------------------
-- HDL Implementation    : Fully Serial
-- Folding Factor        : 16
-- -------------------------------------------------------------
-- Filter Settings:
--
-- Discrete-Time FIR Multirate Filter (real)
-- -----------------------------------------
-- Filter Structure   : Direct-Form FIR Polyphase Decimator
-- Decimation Factor  : 2
-- Polyphase Length   : 33
-- Filter Length      : 65
-- Stable             : Yes
-- Linear Phase       : Yes (Type 1)
--
-- Arithmetic         : fixed
-- Numerator          : s16,15 -> [-1 1)
-- -------------------------------------------------------------



LIBRARY IEEE;
USE IEEE.std_logic_1164.all;
USE IEEE.numeric_std.ALL;

ENTITY rx_ciccomp IS
   PORT( clk                             :   IN    std_logic; 
         clk_enable                      :   IN    std_logic; 
         reset                           :   IN    std_logic; 
         filter_in                       :   IN    std_logic_vector(15 DOWNTO 0); -- sfix16_En15
         filter_out                      :   OUT   std_logic_vector(15 DOWNTO 0); -- sfix16_En15
         ce_out                          :   OUT   std_logic  
         );

END rx_ciccomp;


----------------------------------------------------------------
--Module Architecture: rx_ciccomp
----------------------------------------------------------------
ARCHITECTURE rtl OF rx_ciccomp IS
  -- Local Functions
  -- Type Definitions
  TYPE input_pipeline_type IS ARRAY (NATURAL range <>) OF signed(15 DOWNTO 0); -- sfix16_En15
  -- Constants
  CONSTANT const_one                      : std_logic := '1'; -- boolean
  CONSTANT coeffphase1_1                  : signed(15 DOWNTO 0) := to_signed(1, 16); -- sfix16_En15
  CONSTANT coeffphase1_2                  : signed(15 DOWNTO 0) := to_signed(-1, 16); -- sfix16_En15
  CONSTANT coeffphase1_3                  : signed(15 DOWNTO 0) := to_signed(2, 16); -- sfix16_En15
  CONSTANT coeffphase1_4                  : signed(15 DOWNTO 0) := to_signed(-3, 16); -- sfix16_En15
  CONSTANT coeffphase1_5                  : signed(15 DOWNTO 0) := to_signed(5, 16); -- sfix16_En15
  CONSTANT coeffphase1_6                  : signed(15 DOWNTO 0) := to_signed(-8, 16); -- sfix16_En15
  CONSTANT coeffphase1_7                  : signed(15 DOWNTO 0) := to_signed(12, 16); -- sfix16_En15
  CONSTANT coeffphase1_8                  : signed(15 DOWNTO 0) := to_signed(-18, 16); -- sfix16_En15
  CONSTANT coeffphase1_9                  : signed(15 DOWNTO 0) := to_signed(26, 16); -- sfix16_En15
  CONSTANT coeffphase1_10                 : signed(15 DOWNTO 0) := to_signed(-40, 16); -- sfix16_En15
  CONSTANT coeffphase1_11                 : signed(15 DOWNTO 0) := to_signed(60, 16); -- sfix16_En15
  CONSTANT coeffphase1_12                 : signed(15 DOWNTO 0) := to_signed(-95, 16); -- sfix16_En15
  CONSTANT coeffphase1_13                 : signed(15 DOWNTO 0) := to_signed(159, 16); -- sfix16_En15
  CONSTANT coeffphase1_14                 : signed(15 DOWNTO 0) := to_signed(-293, 16); -- sfix16_En15
  CONSTANT coeffphase1_15                 : signed(15 DOWNTO 0) := to_signed(650, 16); -- sfix16_En15
  CONSTANT coeffphase1_16                 : signed(15 DOWNTO 0) := to_signed(-2120, 16); -- sfix16_En15
  CONSTANT coeffphase1_17                 : signed(15 DOWNTO 0) := to_signed(19710, 16); -- sfix16_En15
  CONSTANT coeffphase1_18                 : signed(15 DOWNTO 0) := to_signed(-2120, 16); -- sfix16_En15
  CONSTANT coeffphase1_19                 : signed(15 DOWNTO 0) := to_signed(650, 16); -- sfix16_En15
  CONSTANT coeffphase1_20                 : signed(15 DOWNTO 0) := to_signed(-293, 16); -- sfix16_En15
  CONSTANT coeffphase1_21                 : signed(15 DOWNTO 0) := to_signed(159, 16); -- sfix16_En15
  CONSTANT coeffphase1_22                 : signed(15 DOWNTO 0) := to_signed(-95, 16); -- sfix16_En15
  CONSTANT coeffphase1_23                 : signed(15 DOWNTO 0) := to_signed(60, 16); -- sfix16_En15
  CONSTANT coeffphase1_24                 : signed(15 DOWNTO 0) := to_signed(-40, 16); -- sfix16_En15
  CONSTANT coeffphase1_25                 : signed(15 DOWNTO 0) := to_signed(26, 16); -- sfix16_En15
  CONSTANT coeffphase1_26                 : signed(15 DOWNTO 0) := to_signed(-18, 16); -- sfix16_En15
  CONSTANT coeffphase1_27                 : signed(15 DOWNTO 0) := to_signed(12, 16); -- sfix16_En15
  CONSTANT coeffphase1_28                 : signed(15 DOWNTO 0) := to_signed(-8, 16); -- sfix16_En15
  CONSTANT coeffphase1_29                 : signed(15 DOWNTO 0) := to_signed(5, 16); -- sfix16_En15
  CONSTANT coeffphase1_30                 : signed(15 DOWNTO 0) := to_signed(-3, 16); -- sfix16_En15
  CONSTANT coeffphase1_31                 : signed(15 DOWNTO 0) := to_signed(2, 16); -- sfix16_En15
  CONSTANT coeffphase1_32                 : signed(15 DOWNTO 0) := to_signed(-1, 16); -- sfix16_En15
  CONSTANT coeffphase1_33                 : signed(15 DOWNTO 0) := to_signed(1, 16); -- sfix16_En15
  CONSTANT coeffphase2_1                  : signed(15 DOWNTO 0) := to_signed(-62, 16); -- sfix16_En15
  CONSTANT coeffphase2_2                  : signed(15 DOWNTO 0) := to_signed(70, 16); -- sfix16_En15
  CONSTANT coeffphase2_3                  : signed(15 DOWNTO 0) := to_signed(-108, 16); -- sfix16_En15
  CONSTANT coeffphase2_4                  : signed(15 DOWNTO 0) := to_signed(159, 16); -- sfix16_En15
  CONSTANT coeffphase2_5                  : signed(15 DOWNTO 0) := to_signed(-225, 16); -- sfix16_En15
  CONSTANT coeffphase2_6                  : signed(15 DOWNTO 0) := to_signed(309, 16); -- sfix16_En15
  CONSTANT coeffphase2_7                  : signed(15 DOWNTO 0) := to_signed(-415, 16); -- sfix16_En15
  CONSTANT coeffphase2_8                  : signed(15 DOWNTO 0) := to_signed(550, 16); -- sfix16_En15
  CONSTANT coeffphase2_9                  : signed(15 DOWNTO 0) := to_signed(-721, 16); -- sfix16_En15
  CONSTANT coeffphase2_10                 : signed(15 DOWNTO 0) := to_signed(941, 16); -- sfix16_En15
  CONSTANT coeffphase2_11                 : signed(15 DOWNTO 0) := to_signed(-1231, 16); -- sfix16_En15
  CONSTANT coeffphase2_12                 : signed(15 DOWNTO 0) := to_signed(1633, 16); -- sfix16_En15
  CONSTANT coeffphase2_13                 : signed(15 DOWNTO 0) := to_signed(-2230, 16); -- sfix16_En15
  CONSTANT coeffphase2_14                 : signed(15 DOWNTO 0) := to_signed(3229, 16); -- sfix16_En15
  CONSTANT coeffphase2_15                 : signed(15 DOWNTO 0) := to_signed(-5282, 16); -- sfix16_En15
  CONSTANT coeffphase2_16                 : signed(15 DOWNTO 0) := to_signed(11541, 16); -- sfix16_En15
  CONSTANT coeffphase2_17                 : signed(15 DOWNTO 0) := to_signed(11541, 16); -- sfix16_En15
  CONSTANT coeffphase2_18                 : signed(15 DOWNTO 0) := to_signed(-5282, 16); -- sfix16_En15
  CONSTANT coeffphase2_19                 : signed(15 DOWNTO 0) := to_signed(3229, 16); -- sfix16_En15
  CONSTANT coeffphase2_20                 : signed(15 DOWNTO 0) := to_signed(-2230, 16); -- sfix16_En15
  CONSTANT coeffphase2_21                 : signed(15 DOWNTO 0) := to_signed(1633, 16); -- sfix16_En15
  CONSTANT coeffphase2_22                 : signed(15 DOWNTO 0) := to_signed(-1231, 16); -- sfix16_En15
  CONSTANT coeffphase2_23                 : signed(15 DOWNTO 0) := to_signed(941, 16); -- sfix16_En15
  CONSTANT coeffphase2_24                 : signed(15 DOWNTO 0) := to_signed(-721, 16); -- sfix16_En15
  CONSTANT coeffphase2_25                 : signed(15 DOWNTO 0) := to_signed(550, 16); -- sfix16_En15
  CONSTANT coeffphase2_26                 : signed(15 DOWNTO 0) := to_signed(-415, 16); -- sfix16_En15
  CONSTANT coeffphase2_27                 : signed(15 DOWNTO 0) := to_signed(309, 16); -- sfix16_En15
  CONSTANT coeffphase2_28                 : signed(15 DOWNTO 0) := to_signed(-225, 16); -- sfix16_En15
  CONSTANT coeffphase2_29                 : signed(15 DOWNTO 0) := to_signed(159, 16); -- sfix16_En15
  CONSTANT coeffphase2_30                 : signed(15 DOWNTO 0) := to_signed(-108, 16); -- sfix16_En15
  CONSTANT coeffphase2_31                 : signed(15 DOWNTO 0) := to_signed(70, 16); -- sfix16_En15
  CONSTANT coeffphase2_32                 : signed(15 DOWNTO 0) := to_signed(-62, 16); -- sfix16_En15
  CONSTANT coeffphase2_33                 : signed(15 DOWNTO 0) := to_signed(0, 16); -- sfix16_En15

  CONSTANT const_zero                     : signed(32 DOWNTO 0) := to_signed(0, 33); -- sfix33_En30
  CONSTANT const_zero_1                   : signed(31 DOWNTO 0) := to_signed(0, 32); -- sfix32_En30
  -- Signals
  SIGNAL cur_count                        : unsigned(4 DOWNTO 0); -- ufix5
  SIGNAL phase_0                          : std_logic; -- boolean
  SIGNAL phase_1                          : std_logic; -- boolean
  SIGNAL phase_1_1                        : std_logic; -- boolean
  SIGNAL phase_16                         : std_logic; -- boolean
  SIGNAL phase_17                         : std_logic; -- boolean
  SIGNAL phase_temp                       : std_logic; -- boolean
  SIGNAL phase_reg_temp                   : std_logic; -- boolean
  SIGNAL phase_reg                        : std_logic; -- boolean
  SIGNAL int_delay_pipe                   : std_logic_vector(0 TO 31); -- boolean
  SIGNAL ce_out_reg                       : std_logic; -- boolean
  SIGNAL input_register                   : signed(15 DOWNTO 0); -- sfix16_En15
  SIGNAL input_pipeline_phase0            : input_pipeline_type(0 TO 32); -- sfix16_En15
  SIGNAL input_pipeline_phase1            : input_pipeline_type(0 TO 32); -- sfix16_En15
  SIGNAL tapsum_0_3and0_29                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_4and0_28                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_6and0_26                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_7and0_25                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_8and0_24                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_9and0_23                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_10and0_22               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_11and0_21               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_12and0_20               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_13and0_19               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_14and0_18               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_0_15and0_17               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_0and1_31                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_1and1_30                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_2and1_29                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_3and1_28                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_4and1_27                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_5and1_26                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_6and1_25                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_7and1_24                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_8and1_23                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_9and1_22                : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_10and1_21               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_11and1_20               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_12and1_19               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_13and1_18               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_14and1_17               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL tapsum_1_15and1_16               : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL input_pipeline_phase016_cast     : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL inputmux                         : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL product                          : signed(32 DOWNTO 0); -- sfix33_En30
  SIGNAL product_mux                      : signed(15 DOWNTO 0); -- sfix16_En15
  SIGNAL phasemux                         : signed(32 DOWNTO 0); -- sfix33_En30
  SIGNAL prod_powertwo_1_1                : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL prod_powertwo_1_2                : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL mulpwr2_temp                     : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL prod_powertwo_1_3                : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL prod_powertwo_1_6                : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL mulpwr2_temp_1                   : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL prod_powertwo_1_28               : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL mulpwr2_temp_2                   : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL prod_powertwo_1_31               : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL prod_powertwo_1_32               : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL mulpwr2_temp_3                   : signed(16 DOWNTO 0); -- sfix17_En15
  SIGNAL prod_powertwo_1_33               : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL powertwo_mux_1_1                 : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL powertwo_mux_1_2                 : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL powertwo_mux_1_3                 : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL powertwo_mux_1_6                 : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL powertwo_mux_1_28                : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL powertwo_mux_1_31                : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL powertwo_mux_1_32                : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL powertwo_mux_1_33                : signed(31 DOWNTO 0); -- sfix32_En30
  SIGNAL sumofproducts                    : signed(40 DOWNTO 0); -- sfix41_En30
  SIGNAL sum_1                            : signed(40 DOWNTO 0); -- sfix41_En30
  SIGNAL add_temp                         : signed(33 DOWNTO 0); -- sfix34_En30
  SIGNAL sum_2                            : signed(40 DOWNTO 0); -- sfix41_En30
  SIGNAL add_temp_1                       : signed(41 DOWNTO 0); -- sfix42_En30
  SIGNAL sum_3                            : signed(40 DOWNTO 0); -- sfix41_En30
  SIGNAL add_temp_2                       : signed(41 DOWNTO 0); -- sfix42_En30
  SIGNAL sum_4                            : signed(40 DOWNTO 0); -- sfix41_En30
  SIGNAL add_temp_3                       : signed(41 DOWNTO 0); -- sfix42_En30
  SIGNAL sum_5                            : signed(40 DOWNTO 0); -- sfix41_En30
  SIGNAL add_temp_4                       : signed(41 DOWNTO 0); -- sfix42_En30
  SIGNAL sum_6                            : signed(40 DOWNTO 0); -- sfix41_En30
  SIGNAL add_temp_5                       : signed(41 DOWNTO 0); -- sfix42_En30
  SIGNAL sum_7                            : signed(40 DOWNTO 0); -- sfix41_En30
  SIGNAL add_temp_6                       : signed(41 DOWNTO 0); -- sfix42_En30
  SIGNAL add_temp_7                       : signed(41 DOWNTO 0); -- sfix42_En30
  SIGNAL sumofproducts_cast               : signed(55 DOWNTO 0); -- sfix56_En30
  SIGNAL acc_sum                          : signed(55 DOWNTO 0); -- sfix56_En30
  SIGNAL accreg_in                        : signed(55 DOWNTO 0); -- sfix56_En30
  SIGNAL accreg_out                       : signed(55 DOWNTO 0); -- sfix56_En30
  SIGNAL add_temp_8                       : signed(56 DOWNTO 0); -- sfix57_En30
  SIGNAL accreg_final                     : signed(55 DOWNTO 0); -- sfix56_En30
  SIGNAL output_typeconvert               : signed(15 DOWNTO 0); -- sfix16_En15
  SIGNAL output_register                  : signed(15 DOWNTO 0); -- sfix16_En15


BEGIN

  -- Block Statements
  Counter : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      cur_count <= to_unsigned(31, 5);
    ELSIF clk'event AND clk = '1' THEN
      IF clk_enable = '1' THEN
        IF cur_count >= to_unsigned(31, 5) THEN
          cur_count <= to_unsigned(0, 5);
        ELSE
          cur_count <= cur_count + to_unsigned(1, 5);
        END IF;
      END IF;
    END IF; 
  END PROCESS Counter;

  phase_0 <= '1' WHEN cur_count = to_unsigned(0, 5) AND clk_enable = '1' ELSE '0';

  phase_1 <= '1' WHEN cur_count = to_unsigned(1, 5) AND clk_enable = '1' ELSE '0';

  phase_1_1 <= '1' WHEN  (((cur_count = to_unsigned(1, 5))  OR
                           (cur_count = to_unsigned(2, 5))  OR
                           (cur_count = to_unsigned(3, 5))  OR
                           (cur_count = to_unsigned(4, 5))  OR
                           (cur_count = to_unsigned(5, 5))  OR
                           (cur_count = to_unsigned(6, 5))  OR
                           (cur_count = to_unsigned(7, 5))  OR
                           (cur_count = to_unsigned(8, 5))  OR
                           (cur_count = to_unsigned(9, 5))  OR
                           (cur_count = to_unsigned(10, 5))  OR
                           (cur_count = to_unsigned(11, 5))  OR
                           (cur_count = to_unsigned(12, 5))  OR
                           (cur_count = to_unsigned(13, 5))  OR
                           (cur_count = to_unsigned(17, 5))  OR
                           (cur_count = to_unsigned(18, 5))  OR
                           (cur_count = to_unsigned(19, 5))  OR
                           (cur_count = to_unsigned(20, 5))  OR
                           (cur_count = to_unsigned(21, 5))  OR
                           (cur_count = to_unsigned(22, 5))  OR
                           (cur_count = to_unsigned(23, 5))  OR
                           (cur_count = to_unsigned(24, 5))  OR
                           (cur_count = to_unsigned(25, 5))  OR
                           (cur_count = to_unsigned(26, 5))  OR
                           (cur_count = to_unsigned(27, 5))  OR
                           (cur_count = to_unsigned(28, 5))  OR
                           (cur_count = to_unsigned(29, 5))  OR
                           (cur_count = to_unsigned(30, 5))  OR
                           (cur_count = to_unsigned(31, 5))  OR
                           (cur_count = to_unsigned(0, 5)))  AND clk_enable = '1') ELSE '0';

  phase_16 <= '1' WHEN cur_count = to_unsigned(16, 5) AND clk_enable = '1' ELSE '0';

  phase_17 <= '1' WHEN cur_count = to_unsigned(17, 5) AND clk_enable = '1' ELSE '0';

  phase_temp <=  phase_0 AND const_one;

  ceout_delay_process : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      int_delay_pipe <= (OTHERS => '0');
    ELSIF clk'event AND clk = '1' THEN
      IF clk_enable = '1' THEN
        int_delay_pipe(1 TO 31) <= int_delay_pipe(0 TO 30);
        int_delay_pipe(0) <= phase_temp;
      END IF;
    END IF;
  END PROCESS ceout_delay_process;
  phase_reg_temp <= int_delay_pipe(31);

  phase_reg <=  phase_reg_temp AND phase_temp;

  ce_out_register_process : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      ce_out_reg <= '0';
    ELSIF clk'event AND clk = '1' THEN
      IF clk_enable = '1' THEN
        ce_out_reg <= phase_reg;
      END IF;
    END IF; 
  END PROCESS ce_out_register_process;

  input_reg_process : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      input_register <= (OTHERS => '0');
    ELSIF clk'event AND clk = '1' THEN
      IF clk_enable = '1' THEN
        input_register <= signed(filter_in);
      END IF;
    END IF; 
  END PROCESS input_reg_process;

  Delay_Pipeline_Phase0_process : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      input_pipeline_phase0(0 TO 32) <= (OTHERS => (OTHERS => '0'));
    ELSIF clk'event AND clk = '1' THEN
      IF phase_0 = '1' THEN
        input_pipeline_phase0(0) <= input_register;
        input_pipeline_phase0(1 TO 32) <= input_pipeline_phase0(0 TO 31);
      END IF;
    END IF; 
  END PROCESS Delay_Pipeline_Phase0_process;

  Delay_Pipeline_Phase1_process : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      input_pipeline_phase1(0 TO 32) <= (OTHERS => (OTHERS => '0'));
    ELSIF clk'event AND clk = '1' THEN
      IF phase_16 = '1' THEN
        input_pipeline_phase1(0) <= input_register;
        input_pipeline_phase1(1 TO 32) <= input_pipeline_phase1(0 TO 31);
      END IF;
    END IF; 
  END PROCESS Delay_Pipeline_Phase1_process;

  -- Adding (or subtracting) the taps based on the symmetry (or asymmetry)

  tapsum_0_3and0_29 <= resize(input_pipeline_phase0(3), 17) + resize(input_pipeline_phase0(29), 17);

  tapsum_0_4and0_28 <= resize(input_pipeline_phase0(4), 17) + resize(input_pipeline_phase0(28), 17);

  tapsum_0_6and0_26 <= resize(input_pipeline_phase0(6), 17) + resize(input_pipeline_phase0(26), 17);

  tapsum_0_7and0_25 <= resize(input_pipeline_phase0(7), 17) + resize(input_pipeline_phase0(25), 17);

  tapsum_0_8and0_24 <= resize(input_pipeline_phase0(8), 17) + resize(input_pipeline_phase0(24), 17);

  tapsum_0_9and0_23 <= resize(input_pipeline_phase0(9), 17) + resize(input_pipeline_phase0(23), 17);

  tapsum_0_10and0_22 <= resize(input_pipeline_phase0(10), 17) + resize(input_pipeline_phase0(22), 17);

  tapsum_0_11and0_21 <= resize(input_pipeline_phase0(11), 17) + resize(input_pipeline_phase0(21), 17);

  tapsum_0_12and0_20 <= resize(input_pipeline_phase0(12), 17) + resize(input_pipeline_phase0(20), 17);

  tapsum_0_13and0_19 <= resize(input_pipeline_phase0(13), 17) + resize(input_pipeline_phase0(19), 17);

  tapsum_0_14and0_18 <= resize(input_pipeline_phase0(14), 17) + resize(input_pipeline_phase0(18), 17);

  tapsum_0_15and0_17 <= resize(input_pipeline_phase0(15), 17) + resize(input_pipeline_phase0(17), 17);

  tapsum_1_0and1_31 <= resize(input_pipeline_phase1(0), 17) + resize(input_pipeline_phase1(31), 17);

  tapsum_1_1and1_30 <= resize(input_pipeline_phase1(1), 17) + resize(input_pipeline_phase1(30), 17);

  tapsum_1_2and1_29 <= resize(input_pipeline_phase1(2), 17) + resize(input_pipeline_phase1(29), 17);

  tapsum_1_3and1_28 <= resize(input_pipeline_phase1(3), 17) + resize(input_pipeline_phase1(28), 17);

  tapsum_1_4and1_27 <= resize(input_pipeline_phase1(4), 17) + resize(input_pipeline_phase1(27), 17);

  tapsum_1_5and1_26 <= resize(input_pipeline_phase1(5), 17) + resize(input_pipeline_phase1(26), 17);

  tapsum_1_6and1_25 <= resize(input_pipeline_phase1(6), 17) + resize(input_pipeline_phase1(25), 17);

  tapsum_1_7and1_24 <= resize(input_pipeline_phase1(7), 17) + resize(input_pipeline_phase1(24), 17);

  tapsum_1_8and1_23 <= resize(input_pipeline_phase1(8), 17) + resize(input_pipeline_phase1(23), 17);

  tapsum_1_9and1_22 <= resize(input_pipeline_phase1(9), 17) + resize(input_pipeline_phase1(22), 17);

  tapsum_1_10and1_21 <= resize(input_pipeline_phase1(10), 17) + resize(input_pipeline_phase1(21), 17);

  tapsum_1_11and1_20 <= resize(input_pipeline_phase1(11), 17) + resize(input_pipeline_phase1(20), 17);

  tapsum_1_12and1_19 <= resize(input_pipeline_phase1(12), 17) + resize(input_pipeline_phase1(19), 17);

  tapsum_1_13and1_18 <= resize(input_pipeline_phase1(13), 17) + resize(input_pipeline_phase1(18), 17);

  tapsum_1_14and1_17 <= resize(input_pipeline_phase1(14), 17) + resize(input_pipeline_phase1(17), 17);

  tapsum_1_15and1_16 <= resize(input_pipeline_phase1(15), 17) + resize(input_pipeline_phase1(16), 17);

  -- Mux(es) to select the input taps for multipliers 

  input_pipeline_phase016_cast <= resize(input_pipeline_phase0(16), 17);

  inputmux <= tapsum_0_3and0_29 WHEN ( cur_count = to_unsigned(1, 5) ) ELSE
                   tapsum_0_4and0_28 WHEN ( cur_count = to_unsigned(2, 5) ) ELSE
                   tapsum_0_6and0_26 WHEN ( cur_count = to_unsigned(3, 5) ) ELSE
                   tapsum_0_7and0_25 WHEN ( cur_count = to_unsigned(4, 5) ) ELSE
                   tapsum_0_8and0_24 WHEN ( cur_count = to_unsigned(5, 5) ) ELSE
                   tapsum_0_9and0_23 WHEN ( cur_count = to_unsigned(6, 5) ) ELSE
                   tapsum_0_10and0_22 WHEN ( cur_count = to_unsigned(7, 5) ) ELSE
                   tapsum_0_11and0_21 WHEN ( cur_count = to_unsigned(8, 5) ) ELSE
                   tapsum_0_12and0_20 WHEN ( cur_count = to_unsigned(9, 5) ) ELSE
                   tapsum_0_13and0_19 WHEN ( cur_count = to_unsigned(10, 5) ) ELSE
                   tapsum_0_14and0_18 WHEN ( cur_count = to_unsigned(11, 5) ) ELSE
                   tapsum_0_15and0_17 WHEN ( cur_count = to_unsigned(12, 5) ) ELSE
                   input_pipeline_phase016_cast WHEN ( cur_count = to_unsigned(13, 5) ) ELSE
                   tapsum_1_0and1_31 WHEN ( cur_count = to_unsigned(17, 5) ) ELSE
                   tapsum_1_1and1_30 WHEN ( cur_count = to_unsigned(18, 5) ) ELSE
                   tapsum_1_2and1_29 WHEN ( cur_count = to_unsigned(19, 5) ) ELSE
                   tapsum_1_3and1_28 WHEN ( cur_count = to_unsigned(20, 5) ) ELSE
                   tapsum_1_4and1_27 WHEN ( cur_count = to_unsigned(21, 5) ) ELSE
                   tapsum_1_5and1_26 WHEN ( cur_count = to_unsigned(22, 5) ) ELSE
                   tapsum_1_6and1_25 WHEN ( cur_count = to_unsigned(23, 5) ) ELSE
                   tapsum_1_7and1_24 WHEN ( cur_count = to_unsigned(24, 5) ) ELSE
                   tapsum_1_8and1_23 WHEN ( cur_count = to_unsigned(25, 5) ) ELSE
                   tapsum_1_9and1_22 WHEN ( cur_count = to_unsigned(26, 5) ) ELSE
                   tapsum_1_10and1_21 WHEN ( cur_count = to_unsigned(27, 5) ) ELSE
                   tapsum_1_11and1_20 WHEN ( cur_count = to_unsigned(28, 5) ) ELSE
                   tapsum_1_12and1_19 WHEN ( cur_count = to_unsigned(29, 5) ) ELSE
                   tapsum_1_13and1_18 WHEN ( cur_count = to_unsigned(30, 5) ) ELSE
                   tapsum_1_14and1_17 WHEN ( cur_count = to_unsigned(31, 5) ) ELSE
                   tapsum_1_15and1_16;

  product_mux <= coeffphase1_4 WHEN ( cur_count = to_unsigned(1, 5) ) ELSE
                      coeffphase1_5 WHEN ( cur_count = to_unsigned(2, 5) ) ELSE
                      coeffphase1_7 WHEN ( cur_count = to_unsigned(3, 5) ) ELSE
                      coeffphase1_8 WHEN ( cur_count = to_unsigned(4, 5) ) ELSE
                      coeffphase1_9 WHEN ( cur_count = to_unsigned(5, 5) ) ELSE
                      coeffphase1_10 WHEN ( cur_count = to_unsigned(6, 5) ) ELSE
                      coeffphase1_11 WHEN ( cur_count = to_unsigned(7, 5) ) ELSE
                      coeffphase1_12 WHEN ( cur_count = to_unsigned(8, 5) ) ELSE
                      coeffphase1_13 WHEN ( cur_count = to_unsigned(9, 5) ) ELSE
                      coeffphase1_14 WHEN ( cur_count = to_unsigned(10, 5) ) ELSE
                      coeffphase1_15 WHEN ( cur_count = to_unsigned(11, 5) ) ELSE
                      coeffphase1_16 WHEN ( cur_count = to_unsigned(12, 5) ) ELSE
                      coeffphase1_17 WHEN ( cur_count = to_unsigned(13, 5) ) ELSE
                      coeffphase2_1 WHEN ( cur_count = to_unsigned(17, 5) ) ELSE
                      coeffphase2_2 WHEN ( cur_count = to_unsigned(18, 5) ) ELSE
                      coeffphase2_3 WHEN ( cur_count = to_unsigned(19, 5) ) ELSE
                      coeffphase2_4 WHEN ( cur_count = to_unsigned(20, 5) ) ELSE
                      coeffphase2_5 WHEN ( cur_count = to_unsigned(21, 5) ) ELSE
                      coeffphase2_6 WHEN ( cur_count = to_unsigned(22, 5) ) ELSE
                      coeffphase2_7 WHEN ( cur_count = to_unsigned(23, 5) ) ELSE
                      coeffphase2_8 WHEN ( cur_count = to_unsigned(24, 5) ) ELSE
                      coeffphase2_9 WHEN ( cur_count = to_unsigned(25, 5) ) ELSE
                      coeffphase2_10 WHEN ( cur_count = to_unsigned(26, 5) ) ELSE
                      coeffphase2_11 WHEN ( cur_count = to_unsigned(27, 5) ) ELSE
                      coeffphase2_12 WHEN ( cur_count = to_unsigned(28, 5) ) ELSE
                      coeffphase2_13 WHEN ( cur_count = to_unsigned(29, 5) ) ELSE
                      coeffphase2_14 WHEN ( cur_count = to_unsigned(30, 5) ) ELSE
                      coeffphase2_15 WHEN ( cur_count = to_unsigned(31, 5) ) ELSE
                      coeffphase2_16;
  product <= inputmux * product_mux;

  phasemux <= product WHEN ( phase_1_1 = '1' ) ELSE
                   const_zero;

  -- Implementing products without a multiplier for coefficients with values equal to a power of 2.

  -- value of 'coeffphase1_1' is 3.0518e-05

  prod_powertwo_1_1 <= resize(input_pipeline_phase0(0), 32);

  -- value of 'coeffphase1_2' is -3.0518e-05

  mulpwr2_temp <= ('0' & input_pipeline_phase0(1)) WHEN input_pipeline_phase0(1) = "1000000000000000"
      ELSE -resize(input_pipeline_phase0(1),17);

  prod_powertwo_1_2 <= resize(mulpwr2_temp, 32);

  -- value of 'coeffphase1_3' is 6.1035e-05

  prod_powertwo_1_3 <= resize(input_pipeline_phase0(2)(15 DOWNTO 0) & '0', 32);

  -- value of 'coeffphase1_6' is -0.00024414

  mulpwr2_temp_1 <= ('0' & input_pipeline_phase0(5)) WHEN input_pipeline_phase0(5) = "1000000000000000"
      ELSE -resize(input_pipeline_phase0(5),17);

  prod_powertwo_1_6 <= resize(mulpwr2_temp_1(16 DOWNTO 0) & '0' & '0' & '0', 32);

  -- value of 'coeffphase1_28' is -0.00024414

  mulpwr2_temp_2 <= ('0' & input_pipeline_phase0(27)) WHEN input_pipeline_phase0(27) = "1000000000000000"
      ELSE -resize(input_pipeline_phase0(27),17);

  prod_powertwo_1_28 <= resize(mulpwr2_temp_2(16 DOWNTO 0) & '0' & '0' & '0', 32);

  -- value of 'coeffphase1_31' is 6.1035e-05

  prod_powertwo_1_31 <= resize(input_pipeline_phase0(30)(15 DOWNTO 0) & '0', 32);

  -- value of 'coeffphase1_32' is -3.0518e-05

  mulpwr2_temp_3 <= ('0' & input_pipeline_phase0(31)) WHEN input_pipeline_phase0(31) = "1000000000000000"
      ELSE -resize(input_pipeline_phase0(31),17);

  prod_powertwo_1_32 <= resize(mulpwr2_temp_3, 32);

  -- value of 'coeffphase1_33' is 3.0518e-05

  prod_powertwo_1_33 <= resize(input_pipeline_phase0(32), 32);

  -- Mux(es) to select the power of 2 products for the corresponding polyphase

  powertwo_mux_1_1 <= prod_powertwo_1_1 WHEN ( phase_1 = '1' ) ELSE
                           const_zero_1;
  powertwo_mux_1_2 <= prod_powertwo_1_2 WHEN ( phase_1 = '1' ) ELSE
                           const_zero_1;
  powertwo_mux_1_3 <= prod_powertwo_1_3 WHEN ( phase_1 = '1' ) ELSE
                           const_zero_1;
  powertwo_mux_1_6 <= prod_powertwo_1_6 WHEN ( phase_1 = '1' ) ELSE
                           const_zero_1;
  powertwo_mux_1_28 <= prod_powertwo_1_28 WHEN ( phase_1 = '1' ) ELSE
                            const_zero_1;
  powertwo_mux_1_31 <= prod_powertwo_1_31 WHEN ( phase_1 = '1' ) ELSE
                            const_zero_1;
  powertwo_mux_1_32 <= prod_powertwo_1_32 WHEN ( phase_1 = '1' ) ELSE
                            const_zero_1;
  powertwo_mux_1_33 <= prod_powertwo_1_33 WHEN ( phase_1 = '1' ) ELSE
                            const_zero_1;

  -- Add the products in linear fashion

  add_temp <= resize(phasemux, 34) + resize(powertwo_mux_1_1, 34);
  sum_1 <= resize(add_temp, 41);

  add_temp_1 <= resize(sum_1, 42) + resize(powertwo_mux_1_2, 42);
  sum_2 <= add_temp_1(40 DOWNTO 0);

  add_temp_2 <= resize(sum_2, 42) + resize(powertwo_mux_1_3, 42);
  sum_3 <= add_temp_2(40 DOWNTO 0);

  add_temp_3 <= resize(sum_3, 42) + resize(powertwo_mux_1_6, 42);
  sum_4 <= add_temp_3(40 DOWNTO 0);

  add_temp_4 <= resize(sum_4, 42) + resize(powertwo_mux_1_28, 42);
  sum_5 <= add_temp_4(40 DOWNTO 0);

  add_temp_5 <= resize(sum_5, 42) + resize(powertwo_mux_1_31, 42);
  sum_6 <= add_temp_5(40 DOWNTO 0);

  add_temp_6 <= resize(sum_6, 42) + resize(powertwo_mux_1_32, 42);
  sum_7 <= add_temp_6(40 DOWNTO 0);

  add_temp_7 <= resize(sum_7, 42) + resize(powertwo_mux_1_33, 42);
  sumofproducts <= add_temp_7(40 DOWNTO 0);

  -- Resize the sum of products to the accumulator type for full precision addition

  sumofproducts_cast <= resize(sumofproducts, 56);

  -- Accumulator register with a mux to reset it with the first addend

  add_temp_8 <= resize(sumofproducts_cast, 57) + resize(accreg_out, 57);
  acc_sum <= add_temp_8(55 DOWNTO 0);

  accreg_in <= sumofproducts_cast WHEN ( phase_17 = '1' ) ELSE
                    acc_sum;

  Acc_reg_process : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      accreg_out <= (OTHERS => '0');
    ELSIF clk'event AND clk = '1' THEN
      IF clk_enable = '1' THEN
        accreg_out <= accreg_in;
      END IF;
    END IF; 
  END PROCESS Acc_reg_process;

  -- Register to hold the final value of the accumulated sum

  Acc_finalreg_process : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      accreg_final <= (OTHERS => '0');
    ELSIF clk'event AND clk = '1' THEN
      IF phase_17 = '1' THEN
        accreg_final <= accreg_out;
      END IF;
    END IF; 
  END PROCESS Acc_finalreg_process;

  output_typeconvert <= resize(shift_right(accreg_final(30 DOWNTO 0) + ( "0" & (accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15) & NOT accreg_final(15))), 15), 16);

  output_register_process : PROCESS (clk, reset)
  BEGIN
    IF reset = '1' THEN
      output_register <= (OTHERS => '0');
    ELSIF clk'event AND clk = '1' THEN
      IF phase_reg = '1' THEN
        output_register <= output_typeconvert;
      END IF;
    END IF; 
  END PROCESS output_register_process;

  -- Assignment Statements
  ce_out <= ce_out_reg;
  filter_out <= std_logic_vector(output_register);
END rtl;
