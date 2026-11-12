namespace Iconiquest.scripts
{
	public partial class Location : Entity
	{
		public static new readonly string groupname = "locations";

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
