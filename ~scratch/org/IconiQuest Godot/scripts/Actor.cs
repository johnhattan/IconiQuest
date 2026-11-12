namespace Iconiquest.scripts
{
	public partial class Actor : Entity
	{
		public static new readonly string groupname = "actors";
		public override void _Ready()
		{
			base._Ready();
			AddToGroup(groupname);

			var tween = GetTree().CreateTween().BindNode(this).SetTrans(Tween.TransitionType.Elastic);
			tween.TweenProperty(this, "modulate", Colors.Red, 1.0f);
			tween.TweenProperty(this, "scale", Vector2.Zero, 1.0f);
			tween.TweenCallback(Callable.From(QueueFree));
		}

		// Called every frame. 'delta' is the elapsed time since the previous frame.
		public override void _Process(double secondsSincePreviousFrame)
		{
			//		Debug.Print("processing ");
		}
	}
}
