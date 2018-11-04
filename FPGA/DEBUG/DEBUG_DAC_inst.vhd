	component DEBUG_DAC is
		port (
			probe : in std_logic_vector(13 downto 0) := (others => 'X')  -- probe
		);
	end component DEBUG_DAC;

	u0 : component DEBUG_DAC
		port map (
			probe => CONNECTED_TO_probe  -- probes.probe
		);

