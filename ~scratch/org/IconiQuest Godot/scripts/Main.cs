namespace Iconiquest.scripts
{
	public partial class Main : NodeSingleton<Main>
	{
		[Export] private Node _worldsContainer = null;
		[Export] public World WorldCurrent { get; protected set; } = null;

		public override void _Ready()
		{
		}

		public override void _Process(double secondsSincePreviousFrame)
		{
		}
	}
}
