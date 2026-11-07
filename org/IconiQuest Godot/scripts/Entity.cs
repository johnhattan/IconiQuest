namespace Iconiquest.scripts
{
	public abstract partial class Entity : Node2D
	{
		[Export] protected Godot.Collections.Dictionary<string, string> _properties = new();
		public PropertyDictionary Properties { get; protected set; } = null;

		public static readonly string groupname = "entities";

		public override void _Ready()
		{
			base._Ready();
			AddToGroup(groupname);
		}

		public override void _Process(double secondsSincePreviousFrame)
		{
			base._Process(secondsSincePreviousFrame);
		}
	}
}