	component DEBUG_Q_RX is
		port (
			probe : in std_logic_vector(15 downto 0) := (others => 'X')  -- probe
		);
	end component DEBUG_Q_RX;

	u0 : component DEBUG_Q_RX
		port map (
			probe => CONNECTED_TO_probe  -- probes.probe
		);

