	component DEBUG_ADC is
		port (
			probe : in std_logic_vector(11 downto 0) := (others => 'X')  -- probe
		);
	end component DEBUG_ADC;

	u0 : component DEBUG_ADC
		port map (
			probe => CONNECTED_TO_probe  -- probes.probe
		);

