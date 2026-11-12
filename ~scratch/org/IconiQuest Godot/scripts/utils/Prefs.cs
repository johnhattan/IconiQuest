namespace Iconiquest.scripts.utils
{
	public class IPrefs
	{ // minimal class that saves, loads, and accesses a dictionary of strings as JSON
		private static Godot.Collections.Dictionary<string, string> _prefs = null;
		private static bool _dirty = false;

		public static readonly string Filename = "user://settings.json";

		private static string _path = null;
		public static string Path
		{
			get
			{
				if (_path == null)
					HasGlobalKey("foo"); // force a load
				return _path;
			}
			private set => _path = value;
		}

		public static bool Autosave = true; // if you set this to false, you'll need to call Flush() manually

		public static void Flush(bool force = true)
		{ // write the prefs to a file
			if (force || (Autosave && _dirty))
			{ // perform the save here
				if (_prefs.Count == 0)
				{ // there are no prefs, so there should be no file. Make sure of it.
					DirAccess da = DirAccess.Open("user://");
					if (da.FileExists(Filename))
						da.Remove(Filename);
					_dirty = false;
				}
				else
				{
					using Godot.FileAccess fa = Godot.FileAccess.Open(Filename, Godot.FileAccess.ModeFlags.Write);
					if (fa != null)
					{
						string s = Json.Stringify(_prefs);
						Path = fa.GetPathAbsolute(); // hang onto the path in case we need to get the file later
						fa.StoreString(s);
						_dirty = false;
					}
				}
			}
		}

		public static bool HasGlobalKey(string key)
		{
			if (_prefs == null)
			{ // lazy-load the prefs file here
				_prefs = new Godot.Collections.Dictionary<string, string>();

				using Godot.FileAccess fa = Godot.FileAccess.Open(Filename, Godot.FileAccess.ModeFlags.Read);
				if (fa != null)
				{
					string s = fa.GetAsText();
					Path = fa.GetPathAbsolute(); // hang onto the path in case we need to get the file later

					Godot.Collections.Dictionary<string, string> d = Json.ParseString(s).AsGodotDictionary<string, string>();
					if (d != null)
					{
						foreach (string k in d.Keys)
							_prefs.Add(k, d[k]);
					}
				}
			}

			return _prefs.ContainsKey(key);
		}

		public static string SetGlobalString(string key, string value)
		{
			if (HasGlobalKey(key))
			{
				if (_prefs[key] != value)
				{
					_prefs[key] = value;
					_dirty = true;
				}
			}
			else
			{
				_prefs.Add(key, value);
				_dirty = true;
			}

			Flush(force: false);
			return value;
		}
		public static string GetGlobalString(string key, string def) => HasGlobalKey(key) ? _prefs[key] : def;

		public static void DeleteGlobalKey(string key)
		{
			if (HasGlobalKey(key))
			{
				_prefs.Remove(key);
				_dirty = true;
			}
			Flush(force: false);
		}
	}

	public class Prefs : IPrefs
	{ // more robust functions built off the minimal iPrefs
		public static int SetGlobalInt(string key, int value)
		{
			SetGlobalString(key, value.ToString());
			return value;
		}
		public static int GetGlobalInt(string key, int defaultValue = 0)
		{
			string s = GetGlobalString(key, null);
			return (s != null) ? s.ParseRelaxed() : defaultValue;
		}

		public static float SetGlobalFloat(string key, float value)
		{
			SetGlobalString(key, value.ToString());
			return value;
		}
		public static float GetGlobalFloat(string key, float defaultValue = 0.0f)
		{
			string s = GetGlobalString(key, null);
			return (s != null) ? s.ParseRelaxedFloat(defaultValue) : defaultValue;
		}

		// everything below this can be built out of this stuff above
		public static int GetGlobalIntClampInclusive(string key, int min, int max, int defaultValue = 0) => GetGlobalInt(key, defaultValue).ClampInclusive(min, max);
		public static int GetGlobalIntClampExclusive(string key, int min, int max, int defaultValue = 0) => GetGlobalInt(key, defaultValue).ClampExclusive(min, max);
		public static float GetGlobalFloatClamp(string key, float min, float max, float defaultValue = 0.0f) => GetGlobalFloat(key, defaultValue).Clamp(min, max);

		public const string UserNameDefault = "User";
		private static string _UserName = null;

		public static string UserName
		{
			get
			{
				_UserName ??= GetGlobalString(UserNameDefault, UserNameDefault + "0");
				return _UserName;
			}
			set
			{
				_UserName = value;
				SetGlobalString(UserNameDefault, _UserName);
			}
		}

		public static int UserNumber
		{ // just convenient to deal with 'em as numbers
			get => UserName.ParseRelaxed();
			set => UserName = UserNameDefault + value.ToString();
		}

		protected static string PrefsPrefix => "@" + UserName + ":";

		public static bool HasKey(string key)
		{
			string[] Types = { PrefsPrefix + "{0}", PrefsPrefix + "Bool:{0}", PrefsPrefix + "Color:{0}-r", PrefsPrefix + "Color32:{0}-r", PrefsPrefix + "Vector2:{0}-x", PrefsPrefix + "Vector3:{0}-x", PrefsPrefix + "Vector4:{0}-x", PrefsPrefix + "Vector3:Quaternion:{0}-x", PrefsPrefix + "Vector4:Rect:{0}-x" };
			bool RetVal = false;
			foreach (string type in Types)
				if (HasGlobalKey(string.Format(type, key)))
				{
					RetVal = true;
					break;
				}

			return RetVal;
		}

		public static void DeleteKey(string key) => DeleteGlobalKey(PrefsPrefix + key);

		public static int SetInt(string key, int value) => SetGlobalInt(PrefsPrefix + key, value);
		public static int GetInt(string key, int defaultValue = 0) => GetGlobalInt(PrefsPrefix + key, defaultValue);
		public static int GetIntClampInclusive(string key, int min, int max, int defaultValue = 0) => GetGlobalIntClampInclusive(PrefsPrefix + key, min, max, defaultValue);
		public static int GetIntClampExclusive(string key, int min, int max, int defaultValue = 0) => GetGlobalIntClampExclusive(PrefsPrefix + key, min, max, defaultValue);

		public static float SetFloat(string key, float value) => SetGlobalFloat(PrefsPrefix + key, value);
		public static float GetFloat(string key, float defaultValue = 0.0f) => GetGlobalFloat(PrefsPrefix + key, defaultValue);
		public static float GetFloatClamp(string key, float min, float max, float defaultValue = 0.0f) => GetGlobalFloatClamp(PrefsPrefix + key, min, max, defaultValue);

		public static string SetString(string key, string value) => SetGlobalString(PrefsPrefix + key, value);
		public static string GetString(string key, string defaultValue = null) => GetGlobalString(PrefsPrefix + key, defaultValue);

		public static void DeleteArrayKey(string key)
		{
			for (var i = GetInt("Array:L:" + key) - 1; i >= 0; --i)
				DeleteKey("Array:" + key + i);
			DeleteKey("Array:L:" + key);
		}

		public static void SetIntArray(string key, int[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetInt("Array:" + key + i, value[i]);
		}
		public static int[] GetIntArray(string key)
		{
			int[] RetVal = new int[GetInt("Array:L:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetInt("Array:" + key + i);
			return RetVal;
		}

		public static void SetFloatArray(string key, float[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetFloat("Array:" + key + i, value[i]);
		}
		public static float[] GetFloatArray(string key)
		{
			float[] RetVal = new float[GetInt("Array:L:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetFloat("Array:" + key + i);
			return RetVal;
		}

		public static void SetStringArray(string key, string[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetString("Array:" + key + i, value[i]);
		}
		public static string[] GetStringArray(string key)
		{
			string[] RetVal = new string[GetInt("Array:L:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetString("Array:" + key + i);
			return RetVal;
		}

		public static bool SetBool(string key, bool value)
		{
			SetInt("bool:" + key, value ? 1 : 0);
			return value;
		}
		public static bool GetBool(string key, bool defaultValue = false)
		{
			bool RetVal = defaultValue;
			int value = GetInt("bool:" + key, 2);
			if (value == 1)
				RetVal = true;
			else if (value == 0)
				RetVal = false;
			return RetVal;
		}

		public static void SetBoolArray(string key, bool[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetBool("Array:" + key + i, value[i]);
		}
		public static bool[] GetBoolArray(string key)
		{
			bool[] RetVal = new bool[GetInt("Array:L:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetBool("Array:" + key + i);
			return RetVal;
		}

		public static Color SetColor(string key, Color value)
		{
			SetFloat("Color:" + key + "-r", value.R);
			SetFloat("Color:" + key + "-g", value.G);
			SetFloat("Color:" + key + "-b", value.B);
			SetFloat("Color:" + key + "-a", value.A);
			return value;
		}

		public static Color GetColor(string key, Color? defaultValue = null)
		{
			if (!defaultValue.HasValue)
				defaultValue = Color.Color8(255, 255, 255, 0); // invisible white

			return new Color
			(
				GetFloat("Color:" + key + "-r", defaultValue.Value.R),
				GetFloat("Color:" + key + "-g", defaultValue.Value.G),
				GetFloat("Color:" + key + "-b", defaultValue.Value.B),
				GetFloat("Color:" + key + "-a", defaultValue.Value.A)
			);
		}

		public static void SetColorArray(string key, Color[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetColor("Array:" + key + i, value[i]);
		}
		public static Color[] GetColorArray(string key)
		{
			Color[] RetVal = new Color[GetInt("Array:L:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetColor("Array:" + key + i);
			return RetVal;
		}

		public static Vector2 SetVector2(string key, Vector2 value)
		{
			SetFloat("Vector2:" + key + "-x", value.X);
			SetFloat("Vector2:" + key + "-y", value.Y);
			return value;
		}
		public static Vector2 GetVector2(string key, Vector2? defaultValue = null)
		{
			if (!defaultValue.HasValue)
				defaultValue = Vector2.Zero;
			return new Vector2
			(
				GetFloat("Vector2:" + key + "-x", defaultValue.Value.X),
				GetFloat("Vector2:" + key + "-y", defaultValue.Value.Y)
			);
		}

		public static void SetVector2Array(string key, Vector2[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetVector2("Array:" + key + i, value[i]);
		}
		public static Vector2[] GetVector2Array(string key)
		{
			Vector2[] RetVal = new Vector2[GetInt("Array:L:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetVector2("Array:" + key + i);
			return RetVal;
		}

		public static Vector3 SetVector3(string key, Vector3 value)
		{
			SetFloat("Vector3:" + key + "-x", value.X);
			SetFloat("Vector3:" + key + "-y", value.Y);
			SetFloat("Vector3:" + key + "-z", value.Z);
			return value;
		}
		public static Vector3 GetVector3(string key, Vector3? defaultValue = null)
		{
			if (!defaultValue.HasValue)
				defaultValue = Vector3.Zero;
			return new Vector3
			(
				GetFloat("Vector3:" + key + "-x", defaultValue.Value.X),
				GetFloat("Vector3:" + key + "-y", defaultValue.Value.Y),
				GetFloat("Vector3:" + key + "-z", defaultValue.Value.Z)
			);
		}

		public static void SetVector3Array(string key, Vector3[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetVector3("Array:" + key + i, value[i]);
		}
		public static Vector3[] GetVector3Array(string key)
		{
			Vector3[] RetVal = new Vector3[GetInt("Array:L:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetVector3("Array:" + key + i);
			return RetVal;
		}

		public static Quaternion SetQuaternion(string key, Quaternion value)
		{
			SetVector3("Quat:" + key, value.GetEuler());
			return value;
		}

		public static Quaternion GetQuaternion(string key, Quaternion? defaultValue = null)
		{
			if (!defaultValue.HasValue)
				defaultValue = Quaternion.Identity;
			return new Quaternion(new Basis(GetVector3("Quaternion:" + key, defaultValue.Value.GetEuler()), 0.0f));
		}

		public static void SetQuatArray(string key, Quaternion[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetQuaternion("Array:" + key + i, value[i]);
		}
		public static Quaternion[] GetQuatArray(string key)
		{
			Quaternion[] RetVal = new Quaternion[GetInt("Array:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetQuaternion("Array:" + key + i);
			return RetVal;
		}

		protected static readonly Rect2 RectEmpty = new Rect2();
		public static Rect2 SetRect2(string key, Rect2 value)
		{
			SetVector2("Rect:" + key + "-position", value.Position);
			SetVector2("Rect:" + key + "-size", value.Size);
			return value;
		}
		public static Rect2 GetRect(string key, Rect2? defaultValue = null)
		{
			if (!defaultValue.HasValue)
				defaultValue = RectEmpty;
			Vector2 pos = GetVector2("Rect:" + key + "-position", defaultValue.Value.Position);
			Vector2 size = GetVector2("Rect:" + key + "-size", defaultValue.Value.Size);
			return new Rect2(pos, size);
		}

		public static void SetRectArray(string key, Rect2[] value)
		{
			SetInt("Array:L:" + key, value.Length);
			for (var i = value.Length - 1; i >= 0; --i)
				SetRect2("Array:" + key + i, value[i]);
		}
		public static Rect2[] GetRectArray(string key)
		{
			Rect2[] RetVal = new Rect2[GetInt("Array:L:" + key)];
			for (var i = RetVal.Length - 1; i >= 0; --i)
				RetVal[i] = GetRect("Array:" + key + i);
			return RetVal;
		}
	}
}