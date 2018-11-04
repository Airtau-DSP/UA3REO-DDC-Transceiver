set_time_format -unit ns -decimal_places 3
create_clock -period 20 -name clk_sys
create_clock -name test_stm32_clk -period 20.0 STM32_CLK

derive_pll_clocks -create_base_clocks
derive_clock_uncertainty
