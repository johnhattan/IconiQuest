namespace Iconiquest.scripts.utils.helpers
{
	public partial class NodeSingleton<T> : Node where T : Node
	{
		public static T Instance { get; protected set; } = null;

		public override void _Ready()
		{
			if (Instance == null)
				Instance = this as T; // first time in, so hang on to my instance
			else
				QueueFree(); // there's already an instance in use, so I should not exist
		}
	}

	public static class MathHelpers
	{
		public static bool Near(this float valToCheck, float val, float tolerance)
		{
			tolerance = Math.Abs(tolerance);
			return (valToCheck >= (val - tolerance)) && (valToCheck <= (val + tolerance));
		}

		public static int ClampInclusive(this int valToCheck, int valMin, int valMax) => Math.Min(Math.Max(valToCheck, valMin), valMax);
		public static int ClampExclusive(this int valToCheck, int valMin, int valMax) => valToCheck.ClampInclusive(valMin, valMax - 1);
		public static float Clamp(this float valToCheck, float valMin, float valMax) => Math.Min(Math.Max(valToCheck, valMin), valMax);

		public static float MaxRelaxed(this float floatToCheck, float compareTo)
		{ // max that allows NaN's (just ignores 'em)
			if (float.IsNaN(floatToCheck))
				return compareTo;
			else if (float.IsNaN(compareTo))
				return floatToCheck;
			else
				return Math.Max(floatToCheck, compareTo);
		}
		public static float MinRelaxed(this float floatToCheck, float compareTo)
		{ // min that allows NaN's (just ignores 'em)
			if (float.IsNaN(floatToCheck))
				return compareTo;
			else if (float.IsNaN(compareTo))
				return floatToCheck;
			else
				return Math.Min(floatToCheck, compareTo);
		}
	}

	public static class GeomHelpers
	{
		public static Rect2 RectContainingAll(this Vector2[] vs, Vector2? _offset = null)
		{ // return a rectangle enclosing all points, offset is if we need to adjust for global
			if (vs.IsNullOrEmpty())
				return new Rect2();
			else
			{
				Vector2 offset = _offset ?? Vector2.Zero;
				Vector2 LT = vs[0] + offset, RB = LT; // start with the first
				for (int index = vs.Length - 1; index >= 1; --index)
				{ // and add in the rest
					Vector2 v = vs[index] + offset;
					if (v.X < LT.X)
						LT.X = v.X;
					else if (v.X > RB.X)
						RB.X = v.X;
					if (v.Y < LT.Y)
						LT.Y = v.Y;
					else if (v.Y > RB.Y)
						RB.Y = v.Y;
				}

				return new Rect2(LT, RB - LT);
			}
		}

		public static Vector2 RotatePointAround(this Vector2 pointToRotate, Vector2 center, double angleInRadians)
		{
			float cosTheta = (float)Math.Cos(angleInRadians);
			float sinTheta = (float)Math.Sin(angleInRadians);
			return new Vector2
			(
				cosTheta * (pointToRotate.X - center.X) - sinTheta * (pointToRotate.Y - center.Y) + center.X,
				sinTheta * (pointToRotate.X - center.X) + cosTheta * (pointToRotate.Y - center.Y) + center.Y
			);
		}
	}

	public static class NodeHelpers
	{
		public static Vector2 PositionInViewport(this Node2D n) => n.GetGlobalTransformWithCanvas().Origin;

		public static Node ChangeParent(this Node nodeToReparent, Node parent)
		{
			Node retVal = nodeToReparent.GetParent();
			retVal?.RemoveChild(nodeToReparent);
			parent.AddChild(nodeToReparent);
			return retVal;
		}

		public static Node[] GetAllChildren(this Node nodeToCheck, int depth = int.MaxValue)
		{ // returns all child nodes to a passed depth
			List<Node> retVal = new();

			if (depth > 0)
				foreach (Node child in nodeToCheck.GetChildren())
				{
					retVal.Add(child);
					if (depth > 1)
						retVal.AddRange(GetAllChildren(child, depth - 1));
				}

			return retVal.ToArray();
		}

		public static T[] GetAllChildrenOfType<T>(this Node nodeToCheck, int depth = int.MaxValue, bool includeSelf = false) where T : Node
		{
			List<T> retVal = new();

			if (includeSelf && (nodeToCheck is T))
				retVal.Add(nodeToCheck as T);

			foreach (Node child in nodeToCheck.GetAllChildren(depth))
				if (child is T)
					retVal.Add(child as T);

			return retVal.ToArray();
		}

		public static Rect2 EnclosingGlobalRect(this Sprite2D s)
		{ // note that this currently assumes centered and a zero offset
			Vector2 center = s.GlobalPosition;
			Vector2 spriteSizeHalf = s.Texture.GetSize() * s.GlobalScale / 2;

			return new Vector2[]
			{ // rotate the corners and return a rect containing all of them
			(center - spriteSizeHalf).RotatePointAround(center, s.GlobalRotation),
			(center + new Vector2(spriteSizeHalf.X, -spriteSizeHalf.Y)).RotatePointAround(center, s.GlobalRotation),
			(center - new Vector2(spriteSizeHalf.X, -spriteSizeHalf.Y)).RotatePointAround(center, s.GlobalRotation),
			(center + spriteSizeHalf).RotatePointAround(center, s.GlobalRotation)
			}.RectContainingAll();
		}

		public static Rect2 EnclosingGlobalRect(this AnimatedSprite2D s)
		{ // note that this currently assumes centered and a zero offset
			Vector2 center = s.GlobalPosition;
			Vector2 spriteSizeHalf = s.SpriteFrames.GetFrameTexture(s.Animation, s.Frame).GetSize() * s.GlobalScale / 2;

			return new Vector2[]
			{ // rotate the corners and return a rect containing all of them
			(center - spriteSizeHalf).RotatePointAround(center, s.GlobalRotation),
			(center + new Vector2(spriteSizeHalf.X, -spriteSizeHalf.Y)).RotatePointAround(center, s.GlobalRotation),
			(center - new Vector2(spriteSizeHalf.X, -spriteSizeHalf.Y)).RotatePointAround(center, s.GlobalRotation),
			(center + spriteSizeHalf).RotatePointAround(center, s.GlobalRotation)
			}.RectContainingAll();
		}

		public static Rect2 EnclosingGlobalRect(this CollisionShape2D cs2D)
		{
			Rect2 retVal = cs2D.Shape.GetRect();
			retVal.Position += cs2D.GlobalPosition;
			return retVal;
		}
	}

	struct Caller
	{
		private DateTime triggerAfter;
		private Action action;
		private int handle;
		private static List<Caller> callers = new();
		static private int handleCurrent = 0; // increment this when you add to the list

		static public bool Busy => callers.Count > 0;

		static public void ProcessHandler()
		{ // call this regularly, like in _Process in the main game loop
			if (callers.Count > 0)
			{
				DateTime now = DateTime.Now;
				for (int index = callers.Count - 1; index >= 0; --index)
					if (now >= callers[index].triggerAfter)
					{
						Action actionToPerform = callers[index].action;
						actionToPerform?.Invoke();
						callers.RemoveAt(index);
					}
			}
		}

		public static int Add(float seconds, Action _action)
		{ // if the delay <= 0, invoke immediately and return a handle not in the list
			int handleToUse = -1;
			if (seconds <= 0.0f)
			{
				_action?.Invoke(); // invoke immediately
			}
			else
			{ // if the delay is > 0, invoke it sometime in the future (Epsilon should be next frame)
				handleToUse = handleCurrent;
				++handleCurrent;
				callers.Add
				(
					new Caller()
					{
						triggerAfter = DateTime.Now + TimeSpan.FromSeconds(seconds),
						action = _action,
						handle = handleToUse
					}
				);
			}

			return handleToUse;
		}

		public static void Add(Action _action) => Add(float.Epsilon, _action); // call on the next handler invoke
		public static void Delay(float seconds) => Add(seconds, null); // this just sets "busy" to true for some amount of time

		public static void Remove(int handle)
		{ // remove a call before it happens. Does nothing if the handle's not there
			for (int i = callers.Count - 1; i >= 0; --i)
				if (callers[i].handle == handle)
					callers.RemoveAt(i);
		}
	};

	public class RndSeeded // randoms should be simple, dagnabbit
	{
		protected readonly Random R = null;

		public RndSeeded(int Seed) => R = new Random(Seed);
		public RndSeeded() => R = new Random();

		public int Int => R.Next(); // returns a random int between 0 and 2,147,483,647
		public int IntUpTo(int max) => R.Next(max); // returns an integer between 0 and max
		public int IntUpToInclusive(int max) => R.Next(max + 1); // returns an integer between 0 and max inclusive
		public int IntBetween(int min, int max) => R.Next(min, max); // returns an integer between min and max
		public int IntBetweenInclusive(int min, int max) => R.Next(min, max + 1); // returns an integer between min and max inclusive

		public float Float => (float)R.NextDouble(); // traditional random number between 0 and 1 exclusive.
		public float FloatBetween(float min, float max) => (Float * (max - min)) + min; // returns a number between min inclusive and max exclusive
		public float FloatUpTo(float max) => FloatBetween(0.0f, max); // returns a number between 0 inclusive and max exclusive

		public bool Bool => (R.Next() & 0x01) == 0; // returns true or false (50% chance of true)
		public bool BoolWeighted(float chance = 0.5f) => (R.NextDouble() < chance); // returns true or false (chance% chance of true)

		public int Sign => ((R.Next() & 0x01) == 0) ? 1 : -1; // returns 1 or -1 (50% chance of 1)
		public int SignWeighted(float chance = 0.5f) => (R.NextDouble() < chance) ? 1 : -1; // returns 1 or -1 (chance% chance of 1)

		public int Bit => R.Next() & 0x01; // returns 1 or 0 (50% chance of 1)
		public int BitWeighted(float chance = 0.5f) => (R.NextDouble() < chance) ? 1 : 0; // returns 1 or 0 (chance% chance of 1)
	}

	public static class Rnd // a global time-seeded randomer for general randomness
	{
		public static readonly RndSeeded RStatic = new();

		public static int Int => RStatic.Int;
		public static int IntUpTo(int max) => RStatic.IntUpTo(max);
		public static int IntUpToInclusive(int max) => RStatic.IntUpToInclusive(max);
		public static int IntBetween(int min, int max) => RStatic.IntBetween(min, max);
		public static int IntBetweenInclusive(int val1, int val2) => (val2 > val1) ? IntUpToInclusive(val2 - val1) + val1 : IntUpToInclusive(val1 - val2) + val2;

		public static float Float => RStatic.Float;
		public static float FloatUpTo(float max) => RStatic.FloatUpTo(max);
		public static float FloatBetween(float min, float max) => RStatic.FloatBetween(min, max);

		public static bool Bool => RStatic.Bool;
		public static bool BoolWeighted(float chance = 0.5f) => RStatic.BoolWeighted(chance);

		public static int Sign => RStatic.Sign;
		public static int SignWeighted(float chance = 0.5f) => RStatic.SignWeighted(chance);

		public static int Bit => RStatic.Bit;
		public static int BitWeighted(float chance = 0.5f) => RStatic.BitWeighted(chance);
	}

	public class RndBag
	{ // random numbers that are guaranteed to go through all values in a bag before repeating
		public int[] Bag { get; protected set; }
		protected int indexCurrent;

		public static int[] CreateSequentialBag(int first, int last)
		{ // create an array of numbers from first to last inclusive
			int[] RetVal = new int[last - first + 1];
			for (var i = 0; i < RetVal.Length; ++i)
				RetVal[i] = i + first;
			return RetVal;
		}

		public RndBag(int[] bagToUse, bool makeCopy = true)
		{ // make a RndBag from any bag you hand it
			if (makeCopy)
			{
				Bag = new int[bagToUse.Length];
				Array.Copy(bagToUse, Bag, bagToUse.Length);
			}
			else
				Bag = bagToUse;

			Bag.Shuffle(); // shuffle the bag so we can use it immediately
			indexCurrent = 0;
		}

		public RndBag(int first, int last) : this(CreateSequentialBag(first, last), false)
		{
		}

		public RndBag(int length) : this(CreateSequentialBag(0, length - 1), false)
		{
		}

		public int Next
		{
			get
			{
				if (indexCurrent >= Bag.Length)
				{ // we made it all the way through, so shuffle and repeat
					Bag.Shuffle();
					indexCurrent = 0;
				}
				return Bag[indexCurrent++];
			}
		}
	}

	internal static class ArrayHelpers
	{
		public static bool IsNullOrEmpty<T>(this T[] array) => (array == null) || (array.Length == 0);

		public static bool InBounds<T>(this T[] array, int index) => (index >= 0) && (index < array.Length);

		public static int IndexOf<T>(this T[] arr, T item) => System.Array.IndexOf(arr, item);

		public static int IndexOfSafe<T>(this T[] arr, T item) => (arr != null) ? arr.IndexOf(item) : -1;

		public static int IndexOfIgnoreCase<T>(this T[] arr, T item)
		{
			int retVal = -1;
			if (item != null)
				retVal = System.Array.FindIndex(arr, m => m.ToString().Equals(item.ToString(), StringComparison.InvariantCultureIgnoreCase));
			return retVal;
		}

		public static void ForEach<T>(this T[] array, Action<T> a) => System.Array.ForEach(array, a);

		public static T[] Shuffle<T>(this T[] array, RndSeeded randomer)
		{ // fisher-yates array shuffler
			int N = array.Length;
			while (N > 1)
			{
				int K = randomer.IntUpTo(N--);
				(array[K], array[N]) = (array[N], array[K]);
			}
			return array;
		}

		public static T[] Shuffle<T>(this T[] array) => Shuffle(array, Rnd.RStatic);
	}

	public static class StringHelpers
	{
		public static bool IsNullOrEmpty(this string str) => string.IsNullOrEmpty(str);

		public static string StripRichText(this string s) => Regex.Replace(s, "<.*?>", string.Empty);

		public static string Escape(this string s) => s.Replace("\\n", "\n");

		public static string Quote(this string s) => "\"" + s + "\"";

		public static string StripCarRet(this string s) => s.Replace("\\n", " ");

		public static string StripFromEnd(this string s, string delim = ".")
		{
			int extPos = s.LastIndexOf(delim, StringComparison.Ordinal);
			return (extPos >= 0) ? s.Substring(0, extPos) : s;
		}

		public static void AppendLineIfNotNull(this StringBuilder s, string sToAdd)
		{ // add another line to this string (with carriage return if there's something already there)
			if (s.Length > 0)
				s.Append('\n');
			s.Append(sToAdd);
		}

		public static readonly string PluralChar = "s";
		public static string PluralStr(this int number, string singular = null, string plural = null) => (number == 1) ? (singular ?? string.Empty) : (plural ?? PluralChar);

		public static bool IsVowel(this string s) => "aeiou".Contains(s[0].ToString(), StringComparison.InvariantCultureIgnoreCase); // if more than one char is in the string, it checks the first

		public static string A(this string noun, bool capitalize = true)
		{
			string prefix = capitalize ? "A" : "a";
			return noun.IsVowel() ? prefix + "n " + noun : prefix + " " + noun;
		}

		public static bool CanParseRelaxed(this string _s) => int.TryParse(new string(_s.Trim().Where(c => (".-0123456789").Contains(c)).ToArray()), out int Ignore); // returns true if this will actually return a value if parsed

		public static int ParseRelaxed(this string _s)
		{   // gets all digits out of a string and parses them. zero if it can't find anything
			int.TryParse(new string(_s.Trim().Where(c => ".-0123456789".Contains(c)).ToArray()), out int retVal);
			return retVal;
		}

		public static float ParseRelaxedFloat(this string _s, float _default = 0.0f)
		{   // gets all digits out of a string and parses them. zero if it can't find anything
			if (!float.TryParse(new string(_s.Trim().Where(c => (".-0123456789").Contains(c)).ToArray()), out float retVal))
				retVal = _default;
			return retVal;
		}

		public static string BoolAsString(this bool b) => b.ToString().ToLowerInvariant();

		public static bool ParseBool(this string _s, bool _default = false)
		{
			bool retVal = _default;
			if (!retVal)
			{
				if ((_s != null) && ((_s == "1") || (_s.ToLowerInvariant() == true.BoolAsString())))
					retVal = true;
			}
			else
			{
				if ((_s != null) && ((_s == "0") || (_s.ToLowerInvariant() == false.BoolAsString())))
					retVal = false;
			}
			return retVal;
		}

		public static bool InRangeInclusive(this char c, char first, char last) => ((c >= first) && (c <= last));

		public static int HashStable(this string s)
		{ // the .NET hash can change from version to version. This one won't
			using (System.Security.Cryptography.MD5 md5 = System.Security.Cryptography.MD5.Create())
			{
				return BitConverter.ToInt32(md5.ComputeHash(Encoding.UTF8.GetBytes(s)), 0);
			}
		}

		public static void ReplaceTag(this StringBuilder sb, string sTag, string sReplaceWith, bool processPlurals = true)
		{ // replaces tags, but with support for plurals and other stuff
			sReplaceWith ??= "";

			int replaceWithInt = sReplaceWith.ParseRelaxed();
			float replaceWithFloat = sReplaceWith.ParseRelaxedFloat();

			if (processPlurals)
			{ // #STag will show an 's' if it is not equal to one
				sb.Replace("#ES" + sTag, (replaceWithInt == 1) ? "" : "es");
				sb.Replace("#S" + sTag, (replaceWithInt == 1) ? "" : "s");
				sb.Replace("#SN" + sTag, (-replaceWithInt == 1) ? "" : "s");
				sb.Replace("#NS" + sTag, (-replaceWithInt == 1) ? "" : "s");
			}

			sb.Replace("#LC" + sTag, sReplaceWith.ToLower()); // lowercase
			sb.Replace("#" + sTag, sReplaceWith);
			sb.Replace("#N" + sTag, (-replaceWithInt).ToString()); // negative
			sb.Replace("#P" + sTag, ((int)(replaceWithFloat * 100.0f)).ToString()); // percent
		}
	}

	public class ObjectDumper
	{
		private int _currentIndent;
		private readonly int _indentSize;
		private readonly StringBuilder _stringBuilder;
		private readonly Dictionary<object, int> _hashListOfFoundElements;
		private readonly char _indentChar;
		private readonly int _depth;
		private int _currentLine;

		private ObjectDumper(int depth, int indentSize, char indentChar)
		{
			_depth = depth;
			_indentSize = indentSize;
			_indentChar = indentChar;
			_stringBuilder = new StringBuilder();
			_hashListOfFoundElements = new Dictionary<object, int>();
		}

		public static string GetDump(object element, int depth = 4, int indentSize = 2, char indentChar = ' ')
		{
			var instance = new ObjectDumper(depth, indentSize, indentChar);
			return instance.DumpElement(element, true);
		}

		private string DumpElement(object element, bool isTopOfTree = false)
		{
			if (_currentIndent > _depth) { return null; }
			if (element == null || element is string)
			{
				Write(FormatValue(element));
			}
			else if (element is ValueType)
			{
				Type objectType = element.GetType();
				bool isWritten = false;
				if (objectType.IsGenericType)
				{
					Type baseType = objectType.GetGenericTypeDefinition();
					if (baseType == typeof(KeyValuePair<,>))
					{
						isWritten = true;
						Write("Key:");
						_currentIndent++;
						DumpElement(objectType.GetProperty("Key").GetValue(element, null));
						_currentIndent--;
						Write("Value:");
						_currentIndent++;
						DumpElement(objectType.GetProperty("Value").GetValue(element, null));
						_currentIndent--;
					}
				}
				if (!isWritten)
				{
					Write(FormatValue(element));
				}
			}
			else
			{
				if (element is IEnumerable enumerableElement)
				{
					foreach (object item in enumerableElement)
					{
						if (item is IEnumerable && !(item is string))
						{
							_currentIndent++;
							DumpElement(item);
							_currentIndent--;
						}
						else
						{
							DumpElement(item);
						}
					}
				}
				else
				{
					Type objectType = element.GetType();
					Write("{{{0}(HashCode:{1})}}", objectType.FullName, element.GetHashCode());
					if (!AlreadyDumped(element))
					{
						_currentIndent++;
						System.Reflection.MemberInfo[] members = objectType.GetMembers(System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.Instance);
						foreach (var memberInfo in members)
						{
							var fieldInfo = memberInfo as System.Reflection.FieldInfo;
							var propertyInfo = memberInfo as System.Reflection.PropertyInfo;

							if (fieldInfo == null && (propertyInfo == null || !propertyInfo.CanRead || propertyInfo.GetIndexParameters().Length > 0))
								continue;

							var type = fieldInfo != null ? fieldInfo.FieldType : propertyInfo.PropertyType;
							object value;
							try
							{
								value = fieldInfo != null
												   ? fieldInfo.GetValue(element)
												   : propertyInfo.GetValue(element, null);
							}
							catch (Exception e)
							{
								Write("{0} failed with:{1}", memberInfo.Name, (e.GetBaseException() ?? e).Message);
								continue;
							}

							if (type.IsValueType || type == typeof(string))
							{
								Write("{0}: {1}", memberInfo.Name, FormatValue(value));
							}
							else
							{
								var isEnumerable = typeof(IEnumerable).IsAssignableFrom(type);
								Write("{0}: {1}", memberInfo.Name, isEnumerable ? "..." : "{ }");

								_currentIndent++;
								DumpElement(value);
								_currentIndent--;
							}
						}
						_currentIndent--;
					}
				}
			}

			return isTopOfTree ? _stringBuilder.ToString() : null;
		}

		private bool AlreadyDumped(object value)
		{
			if (value == null)
				return false;
			if (_hashListOfFoundElements.TryGetValue(value, out int lineNo))
			{
				Write("(reference already dumped - line:{0})", lineNo);
				return true;
			}
			_hashListOfFoundElements.Add(value, _currentLine);
			return false;
		}

		private void Write(string value, params object[] args)
		{
			var space = new string(_indentChar, _currentIndent * _indentSize);

			if (args != null)
				value = string.Format(value, args);

			_stringBuilder.AppendLine(space + value);
			_currentLine++;
		}

		private string FormatValue(object o)
		{
			if (o == null)
				return ("null");

			if (o is DateTime t)
				return t.ToShortDateString();

			if (o is string s)
				return "\"" + s + "\"";

			if (o is char c)
			{
				if (o.Equals('\0'))
				{
					return "''";
				}
				else
				{
					return "'" + c + "'";
				}
			}

			if (o is ValueType)
				return o.ToString();

			if (o is IEnumerable)
				return "...";

			return "{ }";
		}
	}

	public static class CollectionHelpers
	{
		public static bool IsNull(this object o) => o == null;

		public static bool IsNullOrEmpty(this ICollection collection) => collection == null || collection.Count < 1;

		public static List<T> AddRangeIfNotNull<T>(this List<T> haystack, T[] rangeToAdd)
		{
			if (rangeToAdd != null)
				haystack.AddRange(rangeToAdd);
			return haystack;
		}

		public static int? FindRepeatOfLength<T>(this List<T> haystack, int length)
		{
			int? retVal = null;
			if (haystack.Count > length * 2)
			{
				T[] ArrToTest = new T[length];

				for (int i = 0; i < (haystack.Count - length) && !retVal.HasValue; ++i)
				{
					for (int k = 0; k < length; ++k)
						ArrToTest[k] = haystack[i + k];

					for (int k = i + length; (k < haystack.Count - length) && !retVal.HasValue; ++k)
					{
						retVal = k;
						for (int l = 0; (l < length) && retVal.HasValue; ++l)
							if (!ArrToTest[l].Equals(haystack[k + l]))
								retVal = null;
					}
				}
			}
			return retVal;
		}

		public static void Push<T>(this Stack<T> s, Stack<T> sToAdd, bool Reverse = false)
		{ // push a stack onto another stack
			if (Reverse)
				for (var i = 0; i < sToAdd.Count; ++i)
					s.Push(sToAdd.ElementAt(i));
			else
				for (var i = sToAdd.Count - 1; i >= 0; --i)
					s.Push(sToAdd.ElementAt(i));
		}

		public static T GetKey<T>(this OrderedDictionary dictionary, int index)
		{
			if (dictionary.IsNullOrEmpty())
				return default;
			else
			{
				try
				{
					return (T)dictionary.Cast<DictionaryEntry>().ElementAt(index).Key;
				}
				catch (Exception)
				{
					return default;
				}
			}
		}

		public static U GetValue<T, U>(this OrderedDictionary dictionary, T key)
		{
			if (dictionary.IsNullOrEmpty())
				return default;
			else
			{
				try
				{
					return (U)dictionary.Cast<DictionaryEntry>().AsQueryable().Single(kvp => ((T)kvp.Key).Equals(key)).Value;
				}
				catch (Exception)
				{
					return default;
				}
			}
		}
	}
}