	component DEBUG_I_TX is
		port (
			probe : in std_logic_vector(15 downto 0) := (others => 'X')  -- probe
		);
	end component DEBUG_I_TX;

	u0 : component DEBUG_I_TX
		port map (
			probe => CONNECTED_TO_probe  -- probes.probe
		);

