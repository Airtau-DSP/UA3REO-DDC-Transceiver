	component debug1 is
		port (
			probe : in std_logic_vector(15 downto 0) := (others => 'X')  -- probe
		);
	end component debug1;

	u0 : component debug1
		port map (
			probe => CONNECTED_TO_probe  -- probes.probe
		);

