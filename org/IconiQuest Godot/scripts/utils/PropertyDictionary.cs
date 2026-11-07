namespace Iconiquest.scripts.utils
{
	public class PropertyDictionary
	{
		protected Godot.Collections.Dictionary<string, string> propertiesOrg = null;
		protected Godot.Collections.Dictionary<string, string> propertiesCurrent = new();
		protected Godot.Collections.Dictionary<string, string> propertiesNew = new();
		protected Node owner = null; // optional, allows you to treat node attributes like "name" as properties

		public PropertyDictionary(Godot.Collections.Dictionary<string, string> _propertiesOrg, Node _owner = null)
		{ // let the entity/locations pass me a dictionary so you can enter properties in the UI
			if (_propertiesOrg != null)
			{
				propertiesOrg = _propertiesOrg.Duplicate();
				bool visible = false;
				if (_owner != null)
				{ // add a "revealed" member so we can persist the visible attribute
					owner = _owner; // hang on to this
					if (owner is CanvasItem)
						visible = (owner as CanvasItem).Visible;

					string path = "/";
					Node parent = owner.GetParent();
					while (parent != null)
					{
						path = "/" + parent.Name + path;
						parent = parent.GetParent();
					}

					propertiesOrg.Add(EntityNodeNameRO, owner.Name);
					propertiesOrg.Add(EntityFilenameRO, owner.SceneFilePath);
					propertiesOrg.Add(EntityNodePathBaseRO, path);
					propertiesOrg.Add(EntityNodePathFullRO, path + propertiesOrg[EntityNodeNameRO]);
				}
				propertiesOrg.Add(EntityRevealed, visible.BoolAsString());
			}
		}

		// fold the org and new keys together, and throw out the duplicates. Not fast.
		public IEnumerable<string> Keys => propertiesOrg.Keys.Concat(propertiesCurrent.Keys.Concat(propertiesNew.Keys)).Distinct();
		public bool ContainsKey(string key) => (propertiesNew.ContainsKey(key) || propertiesCurrent.ContainsKey(key) || propertiesOrg.ContainsKey(key));

		// these properties are actually part of Node, but you can treat 'em like properties
		public const string EntityName = "name";
		public const string EntityDescription = "description";

		public const string EntityNodeNameRO = "nodeName";
		public const string EntityNodePathBaseRO = "nodePathBase";
		public const string EntityNodePathFullRO = "nodePathFull";
		public const string EntityFilenameRO = "nodeFilename";

		public const string EntityRevealed = "revealed";

		// not all of the above properties can be overridden
		protected static readonly string[] reservedWords = { EntityNodeNameRO, EntityNodePathBaseRO, EntityNodePathFullRO, EntityFilenameRO };

		public string this[string key]
		{
			get
			{ // does not throw an error if there's no key. Just returns null.
				string retVal = null;
				if (!propertiesNew.TryGetValue(key, out retVal))
				{ // no new property, so check the committed properties
					if (!propertiesCurrent.TryGetValue(key, out retVal))
						propertiesOrg.TryGetValue(key, out retVal); // no committed, so check the org
				}

				if ((retVal == null) && (owner != null))
				{ // for convenience, you can get the node string values like they were props
					switch (key)
					{
						case EntityName:
							retVal = owner.Name; // name in the tree
							break;

						case EntityDescription:
							retVal = owner.EditorDescription; // longer description in the node inspector
							break;
					}
				}
				return retVal;
			}

			set
			{
				key = key.ToLowerInvariant();
				if ((key != null) && !reservedWords.Contains(key)) // does nothing if you pass a null or reserved key
				{
					string valueExisting = this[key];

					if (valueExisting != value)
					{
						propertiesNew[key] = value;
						//					if ((key == EntityRevealed) && (owner is CanvasItem))
						//						(owner as CanvasItem).Visible = value.ParseBool(); // setting "visible" changes the appearance

						//					Game.WorldCurrent.MarkWorldChanged(owner);
					}
				}
			}
		}

		public bool AsBool(string key, bool defaultValue = false) => ContainsKey(key) ? this[key].ParseBool(defaultValue) : defaultValue;
		public float AsNum(string key, float defaultValue = 0.0f) => ContainsKey(key) ? this[key].ParseRelaxedFloat(defaultValue) : defaultValue;

		public bool SoundPlayProperty(string key)
		{ // play a property's sound, with any pitch or time mods
			bool retVal = false;
			if (ContainsKey(key))
			{
				StringBuilder sb = new(this[key]);
				//if (ContainsKey(key + "pitch"))
				//					sb.Append(":pitch=" + AsNum(key + "pitch"));
				//				if (ContainsKey(key + "fromtime"))
				//					sb.Append(":fromtime=" + AsNum(key + "fromtime"));
				//				if (ContainsKey(key + "volume"))
				//					sb.Append(":volume=" + AsNum(key + "volume"));

				//			Audio.Play(sb.ToString());
				retVal = true;
			}
			return retVal;
		}

		public bool BoolFlipProperty(string key, bool defaultValue = false)
		{
			bool retVal = !AsBool(key, defaultValue);
			this[key] = retVal.BoolAsString();
			return retVal;
		}

		public void Commit()
		{
			foreach (string key in propertiesNew.Keys)
			{
				string valInNew = propertiesNew[key];
				if (propertiesOrg.TryGetValue(key, out string valInOrg) && (valInOrg == valInNew)) // revert to org
					propertiesCurrent.Remove(key); // value is the same as the org, so just clear the new
				else
					propertiesCurrent[key] = valInNew;
			}
			propertiesNew.Clear();
		}

		public string[] UncommittedProperties => propertiesNew.Keys.ToArray();
	}
}