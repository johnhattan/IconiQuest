namespace Iconiquest.scripts
{
	public partial class Inventory : Location
	{
		public static new readonly string groupname = "inventories";

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