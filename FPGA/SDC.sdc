set_time_format -unit ns -decimal_places 3
create_clock -period 20 -name clk_sys

derive_pll_clocks -create_base_clocks
derive_clock_uncertainty
