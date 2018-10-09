	component debug2 is
		port (
			probe : in std_logic_vector(0 downto 0) := (others => 'X')  -- probe
		);
	end component debug2;

	u0 : component debug2
		port map (
			probe => CONNECTED_TO_probe  -- probes.probe
		);

