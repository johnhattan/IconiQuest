namespace Iconiquest.scripts
{
	public partial class World : Entity
	{
		[Export] protected Location _inventoryLocation = null;
		public Location InventoryLocation { get; protected set; } = null;

		[Export] protected Location _startLocation = null;
		public Location StartLocation { get; protected set; } = null;

		[Export] protected Color _colorBackground = Colors.Black;
		public Color ColorBackground => _colorBackground;

		public Godot.Collections.Array<Location> Locations;
		public Location LocationGet(string name)
		{ // does not throw an error if there's no location matching this. Just returns null.
			Location retVal = null;
			foreach (Location location in Locations)
				if (string.Compare(location.Name, name, StringComparison.OrdinalIgnoreCase) == 0)
				{
					retVal = location;
					break;
				}
			return retVal;
		}

		private static Node[] EmptyNodeArray = Array.Empty<Node>();

		public override void _Process(double secondsSincePreviousFrame)
		{
			base._Process(secondsSincePreviousFrame);
		}

		// Called when the node enters the scene tree for the first time.
		public override void _Ready()
		{
			base._Ready();

			//			StartLocation ??= Locations[0]; // didn't find a primary, so just assume the first is it
		}

		protected Location _locationCurrent;
		public Location LocationCurrent
		{
			get => _locationCurrent;
			set
			{
				_locationCurrent = null;
				foreach (Location locToCheck in Locations)
				{
					if (locToCheck == value)
					{
						_locationCurrent = locToCheck;
						locToCheck.Visible = true;
					}
					else
						locToCheck.Visible = false;
				}

				if (_locationCurrent != null)
				{
					//					float seconds = float.MinValue; // 0 = immediate redirect, < = no redirect
					// do the move here
				}
			}
		}
	}
}