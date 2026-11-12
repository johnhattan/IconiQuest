package;

import openfl.*;
import openfl.display.*;
import openfl.events.*;
import openfl.net.*;
import openfl.geom.*;

import format.SVG;

import motion.easing.*;
import motion.*;

import format.tmx.Data.TmxMap;
import format.tmx.Data.TmxObjectGroup;

using ZManager;
using CZHelpers;

import TextureAtlasSparrow;
import TextureAtlasAngel;
import TextureAtlasRect;

import EntityCustom;

class Pod extends Sprite
{
	private var attachX:Float;	// -1 = left of screen, 0 = floating, 1 = right of screen
	private var attachY:Float;	// -1 = top of screen, 0 = floating, 1 = bottom of screen

	public static inline var amountOnScreen:Float = 64;
	public static inline var tweenTimeDefault:Float = 0.15;

	public var colorBackground:Int;
	public var colorOutline:Int;
	public static var podExpanded(default, null):Pod = null;
	public static var pods(default, null):Array<Pod> = [];

	public function drawRoundRectComplex(_g:Graphics, x:Float, y:Float, w:Float, h:Float, rTL:Float, rTR:Float, rBL:Float, rBR:Float)
	{
		var max:Float = (w < h ? w : h) / 2, mTL:Float = 0, mTR:Float = 0, mBR:Float = 0, mBL:Float = 0;

		if (rTL < 0)
			rTL *= (mTL = -1);

		if (rTL > max)
			rTL = max;

		if (rTR < 0)
			rTR *= (mTR = -1);

		if (rTR > max)
			rTR = max;

		if (rBR < 0)
			rBR *= (mBR = -1);

		if (rBR > max)
			rBR = max;

		if (rBL < 0)
			rBL *= (mBL = -1);

		if (rBL > max)
			rBL = max;

		_g.moveTo(x + w - rTR, y);
		_g.curveTo(x + w + rTR * mTR, y - rTR * mTR, x + w, y + rTR);
		_g.lineTo(x + w, y + h - rBR);
		_g.curveTo(x + w + rBR * mBR, y + h + rBR * mBR, x + w - rBR, y + h);
		_g.lineTo(x + rBL, y + h);
		_g.curveTo(x - rBL * mBL, y + h + rBL * mBL, x, y + h - rBL);
		_g.lineTo(x, y + rTL);
		_g.curveTo(x - rTL * mTL, y - rTL * mTL, x + rTL, y);
	}

	public function reposition(time:Float = 0)
	{
		var xNew:Float = x;
		var yNew:Float = y;
		if (podExpanded == this)
		{	// show the pod as expanded
			if ((attachX == 0) && (attachY == 0))
				visible = true;	// floating pods are not automatically positioned
			else
			{
				if (attachX == -1)
					xNew = 0;
				else if (attachX == 1)
					xNew = Game.instance.bounds.width - width + 1;

				if (attachY == -1)
					yNew = 0;
				else if (attachY == 1)
					yNew = Game.instance.bounds.height - height + 1;
			}
		}
		else
		{
			if ((attachX == 0) && (attachY == 0))
				visible = false;	// this is a floating pod, and it's only visible when expanded
			else
			{
				if (attachX == -1)
					xNew = - width + amountOnScreen;
				else if (attachX == 1)
					xNew = Game.instance.bounds.width - amountOnScreen;

				if (attachY == -1)
					yNew = - height + amountOnScreen;
				else if (attachY == 1)
					yNew = Game.instance.bounds.height - amountOnScreen;
			}
		}
		
		if ((x != xNew) || (y != yNew))
		{
			if (time != 0)
			{
				Actuate.tween
				(
					this,
					time,
					{
						x:xNew,
						y:yNew
					},
					false
				)
				.ease(Linear.easeNone);
			}
			else
			{
				x = xNew;
				y = yNew;
			}
		}
	}

	static public inline function retractAll(time:Float = 0)
	{
		if (podExpanded != null)
			podExpanded.expand(false, time);
	}

	public inline function expanded():Bool
		return (podExpanded == this);

	public function expand(_b:Bool, time:Float = 0)
	{
		if (_b)
		{
			if (podExpanded != this)
			{
				retractAll(time);
				podExpanded = this;
				reposition(time);
			}
		}
		else
		{
			if (podExpanded == this)
			{	// make the retract happen
				podExpanded = null;
				reposition(time);
			}
		}
		return _b;
	}

	public function new (_width:Float, _height:Float, _attachX:Float = 0, _attachY:Float = 0, _colorBackground:Int = 0xB0B0B0, _colorOutline:Int = 0xB0B0B0)
	{
		colorBackground = _colorBackground;
		colorOutline = _colorOutline;
		attachX = _attachX;
		attachY = _attachY;

		super();

		graphics.clear();
		graphics.lineStyle(2.5, colorOutline);
		graphics.beginFill(colorBackground);

		var rTL:Float = amountOnScreen / 2;
		var rTR:Float = amountOnScreen / 2;
		var rBL:Float = amountOnScreen / 2;
		var rBR:Float = amountOnScreen / 2;
		if (_attachX == -1)
			rTL = rBL = 0;
		else if (_attachX == 1)
			rTR = rBR = 0;
		if (_attachY == -1)
			rTL = rTR = 0;
		else if (_attachY == 1)
			rBL = rBR = 0;
		drawRoundRectComplex(graphics, 0, 0, _width, _height, rTL, rTR, rBL, rBR);
		graphics.endFill();
		
		pods.push(this);
	}
}

class NotePod extends Pod
{
	private var posActor:Point;
	private var posCenterScreen:Point;

	private var widthPod:Float;
	private var heightPod:Float;

	public static inline var maxNotes:Int = 4;

	public var actor(null, set):Actor;
	public function set_actor(_a:Actor):Actor
	{
		actor = _a;
		graphics.clear();
		graphics.lineStyle(2.5, colorOutline);
		graphics.beginFill(colorBackground);

		var radiusTL, radiusTR, radiusBL, radiusBR:Float;
		radiusTL = radiusTR = radiusBL = radiusBR = Pod.amountOnScreen / 2;
		if (actor != null)
		{	// and draw a caret in the right place
			var worldNotes:World = World.world('notes');
			widthPod = actor.propertiesGet('note').length * worldNotes.widthTile;
			heightPod = worldNotes.heightTile;

			reposition();	// put me in the right place now that I have width and height

			if (posActor.x < posCenterScreen.x)	// now place the pointy corner
			{	// left
				if (posActor.y < posCenterScreen.y)
					radiusTL = 0;
				else
					radiusBL = 0;
			}
			else
			{	// right
				if (posActor.y < posCenterScreen.y)
					radiusTR = 0;
				else
					radiusBR = 0;
			}
		}

		drawRoundRectComplex(graphics, 0, 0, widthPod, heightPod, radiusTL, radiusTR, radiusBL, radiusBR);
		graphics.endFill();
	
		return _a;
	}

	override public function reposition(time:Float = 0)
	{
		if (actor != null)
		{	// position our note relative to an actor, with carets and stuff
			posActor = actor.sprite.parent.localToGlobal(new Point(actor.sprite.x, actor.sprite.y));
			posCenterScreen = new Point(Game.instance.bounds.width / 2, Game.instance.bounds.height / 2);
			var ptActorGlobalEdge:Point = actor.sprite.parent.localToGlobal(new Point(actor.sprite.x+actor.sprite.width/2, actor.sprite.y));
			var gutter = (ptActorGlobalEdge.x - posActor.x) * 0.6;

			if (posActor.x < posCenterScreen.x)
				x = posActor.x + gutter;
			else
				x = posActor.x - widthPod - gutter;

			if (posActor.y < posCenterScreen.y)
				y = posActor.y + gutter;
			else
				y = posActor.y - heightPod - gutter;
		}
		super.reposition(time);
	}
	
	public function new (_width:Float, _height:Float, _attachX:Float = 0, _attachY:Float = 0)
		super(_width, _height, _attachX, _attachY, 0xFFFFFF, 0x000000);
}

class GlyphCache
{
	private var SVGs:Map<String, SVG>;
	private var BDs:Map<String, BitmapData>;
	public var atlasSparrow(null, default):TextureAtlasSparrowCached;
	public var atlasAngel(null, default):TextureAtlasAngelCached;

	private function createEntry(_s:String)
	{
		var svg:SVG = null;
		var bd:BitmapData = null;

		var pathSVG:String = 'assets/' + _s + '.svg';
		if (Assets.exists(pathSVG))
			svg = new SVG (Assets.getText (pathSVG));

		if (svg == null)
		{	// no SVG in the assets, so get a bitmap if we can
			if (bd == null)
			{
				var pathPNG:String = 'assets/' + _s + '.png';
				if (Assets.exists(pathPNG))
					bd = Assets.getBitmapData(pathPNG);
			}

			if ((bd == null) && (atlasSparrow != null))
			{	// see if it's in the sparrow atlas
				var index:Int = atlasSparrow.indexOf(_s);
				if (index != -1)
					bd = atlasSparrow.createBitmapDataCached(index);
			}
		}

		SVGs[_s] = svg;
		BDs[_s] = bd;
	}

	public inline function getSVG(_s:String):SVG
	{
		if (!SVGs.exists(_s))
			createEntry(_s);
		return SVGs[_s];
	}

	public inline function getBD(_s:String, _asText:Bool = false):BitmapData
	{
		var retVal:BitmapData = null;
		if (_asText)
		{	// make a text string outta this. Don't cache it because strings can change a lot
			if (atlasAngel != null)
				retVal = atlasAngel.createBitmapDataStrCR(_s, true);
		}
		else
		{
			if (!BDs.exists(_s))
				createEntry(_s);
			retVal = BDs[_s];
		}
		return retVal;
	}

	public static var instance(default, null):GlyphCache = null;
	public static function createInstanceStatic():GlyphCache
	{
		if (instance == null)
			instance = new GlyphCache();
		return instance;
	}

	public function new()
	{
		atlasSparrow = null;
		atlasAngel = null;
		SVGs = new Map<String, SVG>();
		BDs = new Map<String, BitmapData>();
	}
}

class Entity
{	// an entity is an abstract thing with a name and (optional) parameters
	private var entityParent:Entity;
	private var propertiesOrg:Map<String, String>;
	private var propertiesNew:Map<String, String>;

	private static inline var strMarkerDereference:String = '.';
	private static inline var strMarkerText:String = '#';

	public var name(get, set):String;
	public inline function get_name():String
		return propertyGet('name');
	public inline function set_name(_n:String):String
		return propertySet('name', _n);

	public var nameOrg(get, never):String;
	public inline function get_nameOrg():String
		return propertyGetOrg('name');

	public inline function propertyExists(_prop:String):Bool
		return propertiesOrg.exists(_prop) || propertiesNew.exists(_prop);

	public inline function propertyGetOrg(_prop:String, _default:String = null):String
		return (propertiesOrg.exists(_prop)) ? propertiesOrg[_prop] : _default;

	public inline function propertyGet(_prop:String, _default:String = null):String
		return (propertiesNew.exists(_prop)) ? propertiesNew[_prop] : propertyGetOrg(_prop, _default);

	public inline function propertiesCommit()
	{	// commit all new properties to org
		for (key in propertiesNew.keys())
			propertiesOrg[key] = propertiesNew[key];
		propertiesNew = new Map<String, String>();
	}

	public function propertiesGet(_s:String):Array<String>
	{	// makes a list of things. First item is "item", next is "item1", "item2", etc
		var retVal:Array<String> = propertyExists(_s) ? [] : null;

		if (retVal != null)
		{
			var valToCheck:String = propertyGet(_s);
			var next:Int = 0;

			while (valToCheck != null)
			{
				retVal.push(valToCheck);
				valToCheck = propertyGet(_s + Std.string(++next));
			}
		}

		return retVal;
	}

	public inline function propertyGetOrgAsInt(_prop:String, _default:Int = 0):Int
	{
		var val:String = propertyGetOrg(_prop);
		return (val == null) ? _default : Std.parseInt(val);
	}

	public inline function propertyGetOrgAsFloat(_prop:String, _default:Float = 0.0):Float
	{
		var val:String = propertyGetOrg(_prop);
		return (val == null) ? _default : Std.parseFloat(val);
	}

	public inline function propertyGetAsInt(_prop:String, _default:Int = 0):Int
	{
		var val:String = propertyGet(_prop);
		return (val == null) ? _default : Std.parseInt(val);
	}

	public function propertiesGetAsInt(_s:String):Array<Int>
	{	// makes a list of things. First item is "item", next is "item1", "item2", etc
		var arr:Array<String> = propertiesGet(_s);
		return (arr != null) ? arr.map(Std.parseInt) : null;
	}

	public inline function propertyGetAsFloat(_prop:String, _default:Float = 0.0):Float
	{
		var val:String = propertyGet(_prop);
		return (val == null) ? _default : Std.parseFloat(val);
	}

	public function propertiesGetAsFloat(_s:String):Array<Float>
	{	// makes a list of things. First item is "item", next is "item1", "item2", etc
		var arr:Array<String> = propertiesGet(_s);
		return (arr != null) ? arr.map(Std.parseFloat) : null;
	}

	public function propertyGetOrgAsBool(_prop:String, _default:Bool = false):Bool
	{
		var retVal:Bool = _default;
		var p:String = propertyGetOrg(_prop);
		
		if (retVal == false)
		{
			if ((p != null) && ((p == '1') || (p.toLowerCase() == 'true')))
				retVal = true;
		}
		else
		{
			if ((p != null) && ((p == '0') || (p.toLowerCase() == 'false')))
				retVal = false;
		}
		return retVal;
	}

	public function propertyGetAsBool(_prop:String, _default:Bool = false):Bool
	{
		var retVal:Bool = _default;
		var p:String = propertyGet(_prop);
		
		if (retVal == false)
		{
			if ((p != null) && ((p == '1') || (p.toLowerCase() == 'true')))
				retVal = true;
		}
		else
		{
			if ((p != null) && ((p == '0') || (p.toLowerCase() == 'false')))
				retVal = false;
		}
		return retVal;
	}

	public function propertySet(_prop:String, _propValue:String):String
	{
		if (propertiesOrg[_prop] == _propValue)
			propertiesNew.remove(_prop);	// setting back to the default, so just remove the new property
		else
			propertiesNew[_prop] = _propValue;	// add/change the new value
		return _propValue;
	}

	public inline function propertySetAsInt(_prop:String, _propValue:Int):Int
	{
		propertySet(_prop, Std.string(_propValue));
		return _propValue;
	}

	public inline function propertySetAsFloat(_prop:String, _propValue:Float):Float
	{
		propertySet(_prop, Std.string(_propValue));
		return _propValue;
	}

	public inline function propertySetAsBool(_prop:String, _propValue:Bool):Bool
	{
		propertySet(_prop, Std.string(_propValue ? 'true' : 'false'));
		return _propValue;
	}

	public inline function propertyFlipBool(_prop:String, _default:Bool = false):Bool
		return propertySetAsBool(_prop, !propertyGetAsBool(_prop, _default));

	public function toXML(prefix:String = ''):Xml
	{
		var retVal:Xml = Xml.createElement(prefix+nameOrg);
		for (key in propertiesNew.keys())
			retVal.set(key, propertiesNew[key]);

		return retVal;
	}

	public function soundPlayEvent(_s:String):Bool
	{
		var retVal:Bool = false;
		if (propertyExists(_s))
		{
			Sounds.play
			(
				propertyGet(_s),
				propertyGetAsInt(_s + 'NumTimes', 1),
				propertyGetAsFloat(_s + 'Position', 0),
				propertyGetAsFloat(_s + 'Delay', 0),
				propertyGetAsFloat(_s + 'Duration', 0)
			);
			retVal = true;
		}
		return retVal;
	}

	public function soundPlayThemeEvent(_s:String):Bool
	{
		var retVal:Bool = false;
		if (propertyExists(_s))
		{
			Sounds.playTheme
			(
				propertyGet(_s),
				propertyGetAsInt(_s + 'NumTimes', 1),
				propertyGetAsFloat(_s + 'Position', 0)
			);
			retVal = true;
		}
		return retVal;
	}

	private function revert()	// kill all new properties, returning the object to its original state
		propertiesNew = new Map<String, String>();

	private function new (_name:String = '', _entityParent:Entity = null, _properties:Map<String, String> = null)
	{
		entityParent = _entityParent;
		propertiesOrg = new Map<String, String>();
		propertiesOrg['name'] = _name;
		if (_properties != null)	// deep copy the map
			for (key in _properties.keys())
				propertiesOrg[key] = _properties[key];

		propertiesNew = new Map<String, String>();
	}
}

class Actor extends Entity
{	// an actor is an entity that you can see and interact with
	public var sprite:Sprite;

	static private var revealActorsCache:Map<String, Array<Actor>> = new Map<String, Array<Actor>>();

	public var locationParent(get, never):Location;
	public inline function get_locationParent():Location
		return cast entityParent;

	public var worldParent(get, never):World;	// actors can't elect to change their world. That's gotta happen top-down
	public inline function get_worldParent():World
		return cast entityParent.entityParent;

	// hidden, x, y, xOrg, yOrg pretend to be members, but they're really properties
	public var hidden(get, set):Bool;
	public function get_hidden():Bool
	{
		var retVal:Bool = false;
		if (propertyExists('hiddenUnlessActor'))
		{
			if (locationParent.actor(propertyGet('hiddenUnlessActor')) == null)
				retVal = true;
		}
		else
			retVal = propertyGetAsBool('hidden', false);

		return retVal;
	}
	public inline function set_hidden(_b:Bool):Bool
	{	// note that hiddenUnlessActor overrides hidden
		if (!propertyExists('hiddenUnlessActor'))
			propertySetAsBool('hidden', _b);
		return _b;
	}

	public var x(get, set):Float;
	public inline function get_x():Float
		return propertyGetAsFloat('x');
	public inline function set_x(_f:Float):Float
		return propertySetAsFloat('x', _f);

	public var y(get, set):Float;
	public inline function get_y():Float
		return propertyGetAsFloat('y');
	public inline function set_y(_f:Float):Float
		return propertySetAsFloat('y', _f);

	public var xOrg(get, never):Float;
	public inline function get_xOrg():Float
		return propertyGetOrgAsFloat('x');

	public var yOrg(get, never):Float;
	public inline function get_yOrg():Float
		return propertyGetOrgAsFloat('y');

	private var glyphExisting:String = null;

	public var glyph(get, set):String;
	public inline function get_glyph():String	// the glyph defaults to the name
		return propertyExists('glyph') ? propertyGet('glyph') : name;
	public function set_glyph(_glyphNew:String):String
	{
		if ((glyphExisting == null) || (glyphExisting != _glyphNew))
		{	// first time in, or the glyph name actually changed
			sprite.graphics.clear();
			if (propertyGetAsBool('drawBackground', true))
			{
				var svg:SVG = GlyphCache.instance.getSVG('_blank');
				if (svg != null)
					svg.render(sprite.graphics, -svg.data.width / 2, -svg.data.height / 2, Std.int(worldParent.widthTile), Std.int(worldParent.heightTile));
				else
				{	// no SVG, so try a bitmap
					var bd:BitmapData = GlyphCache.instance.getBD('_blank');
					if (bd != null)
					{	// draw this bitmap to the sprite
						var mat:Matrix = new Matrix();
						mat.scale(worldParent.widthTile / bd.width, worldParent.heightTile / bd.height);
						mat.translate(-worldParent.widthTile / 2, -worldParent.heightTile / 2);
						sprite.graphics.beginBitmapFill(bd, mat, true, true);
						sprite.graphics.drawRect(-worldParent.widthTile/2, -worldParent.heightTile/2 , worldParent.widthTile, worldParent.heightTile);
					}
					else
					{	// a gray citcle filled with white
						sprite.graphics.lineStyle(Std.parseFloat(propertyGet('widthLine', '1.5')), Std.parseInt(propertyGet('colorLine', '0x808080')));
						sprite.graphics.beginFill(Std.parseInt(propertyGet('colorBackground', '0xAAAAAA')));
						sprite.graphics.drawEllipse(-worldParent.widthTile/2, -worldParent.heightTile/2, worldParent.widthTile, worldParent.heightTile);
						sprite.graphics.endFill();
					}
				}
			}
			else
			{
				sprite.graphics.lineStyle(0, 0, 0);	// an invisible line so the dimensions still happen
				sprite.graphics.drawEllipse(-worldParent.widthTile/2, -worldParent.heightTile/2, worldParent.widthTile, worldParent.heightTile);
			}

			var doGlyph:DisplayObject = null;
			var widthTarget = sprite.width - 4;
			var heightTarget = sprite.height - 4;

			if ((_glyphNew == null) || (_glyphNew.toLowerCase() == 'blank'))
			{
				if (_glyphNew == null)
					_glyphNew = 'blank';
			}
			else if ((_glyphNew.charAt(0) == Entity.strMarkerText) && (_glyphNew.charAt(_glyphNew.length - 1) == Entity.strMarkerText))
			{
				var strToWrite:String = _glyphNew.substring(1, _glyphNew.length - 1);
				var bd:BitmapData = GlyphCache.instance.getBD(strToWrite, true);
				if (bd != null)
				{
					var b:Bitmap = new Bitmap(bd, PixelSnapping.AUTO, true);
					var pngScale:Float = heightTarget / b.height;
					b.scaleX = b.scaleY = pngScale;
					b.x = -b.width / 2;
					b.y = -b.height / 2;
					doGlyph = b;
				}
			}
			else
			{	// first, check for a svg
				var done:Bool = false;
				while (!done)
				{
					var svg:SVG = GlyphCache.instance.getSVG(_glyphNew);
					if (svg != null)
					{
						var s:Shape = new Shape();
						svg.render(s.graphics, -svg.data.width / 2, -svg.data.height / 2);
						var svgScale:Float = (s.width > s.height) ? widthTarget / s.width : heightTarget / s.height;
						s.scaleX = s.scaleY = svgScale;
						doGlyph = s;
					}
					else
					{
						var bd:BitmapData = GlyphCache.instance.getBD(_glyphNew);
						if (bd != null)
						{
							var b:Bitmap = new Bitmap(bd, PixelSnapping.AUTO, true);
							var pngScale:Float = (b.width > b.height) ? widthTarget / b.width : heightTarget / b.height;
							b.scaleX = b.scaleY = pngScale;
							b.x = -b.width / 2;
							b.y = -b.height / 2;
							doGlyph = b;
						}
					}
				
					if (doGlyph != null)
						done = true;
					else
					{
						done = true;
						if (propertiesOrg.exists('glyph'))
						{
							var g:String = propertiesOrg['glyph'];
							if (g != _glyphNew)
							{
								_glyphNew = g;
								done = false;
							}
						}
						else
						{
							var g:String = nameOrg;
							if (g != _glyphNew)
							{
								_glyphNew = g;
								done = false;
							}
						}
					}
				}
			}

			glyphExisting = _glyphNew;
			if (glyphExisting != glyph)
				propertySet('glyph', glyphExisting);

			sprite.removeChildren();
			if (doGlyph != null)
				sprite.addChild(doGlyph);
		}

		return _glyphNew;
	}

	private function updateRevealCache(revealActors:Array<String>)
	{	// first, create new entries in the cache for actors
		var mustClean:Bool = false;
		for (revealActorName in revealActors)
		{
			if (revealActorsCache.exists(revealActorName))
				mustClean = true;	// there's an entry already, so we'll have to check if it's clean
			else
				revealActorsCache[revealActorName] = [ this ];
		}

		if (mustClean)
		{	// we updated where there's already an entry, so let's make sure it's nice and neat
			for (actorName in revealActorsCache.keys())
			{
				var revealers = revealActorsCache[actorName];
				var revealerIndex:Int = revealers.indexOf(this);
				if (revealActors.indexOf(actorName) != -1)
				{	// actor is in the cache, so make sure its list contains me
					if (revealerIndex == -1)
						revealers.push(this);
				}
				else
				{	// this entry should be removed if it's there
					if (revealerIndex != -1)
					{
						revealers.remove(this);	// remove this from the cache, as it doesn't belong here
						if (revealers.length == 0)
							revealActorsCache.remove(actorName);	// there's nothing left, so purge the whole entry
					}
				}
			}
		}
	}
	
	public function isFullyRevealed():Bool
	{
		var retVal:Bool = true;

		var revealActors:Array<String> = propertiesGet('revealActor');
		var revealActorsSync:Array<String> = propertiesGet('revealActorSync');
		if ((revealActors != null) || (revealActorsSync != null))
		{
			if (revealActors == null)
				revealActors = revealActorsSync;
			else if (revealActorsSync != null)
				revealActors = revealActors.concat(revealActorsSync);

			for (revealActor in revealActors)
			{
				var revealers:Array<Actor> = revealActorsCache[revealActor];
				for (revealer in revealers)
					if (!revealer.propertyGetAsBool(revealActor + 'Revealed'))
					{	// something's not revealed, so bail
						retVal = false;
						break;
					}
				
				if (!retVal)
					break;	// we're done
			}
		}
		return retVal;
	}

	public function onPlace()
	{
		var revealActors:Array<String> = propertiesGet('revealActor');
		var revealActorsSync:Array<String> = propertiesGet('revealActorSync');

		if ((revealActors != null) || (revealActorsSync != null))
		{
			if (revealActors == null)
				revealActors = revealActorsSync;
			else if (revealActorsSync != null)
				revealActors = revealActors.concat(revealActorsSync);

			updateRevealCache(revealActors);

			var xDelta:Float = propertyGetAsFloat('revealDeltaX', 0);
			var yDelta:Float = propertyGetAsFloat('revealDeltaY', 0);
			for (revealActor in revealActors)
			{
				var revealers:Array<Actor> = revealActorsCache[revealActor];
				var isRevealed:Bool = propertyGetAsBool(revealActor + 'Revealed');
				var isAllRevealed:Bool = true;

				for (revealer in revealers)
					if (!revealer.propertyGetAsBool(revealActor + 'Revealed'))
					{	// not revealed
						isAllRevealed = false;
						break;
					}

				if ((xDelta != 0) || (yDelta != 0))
				{	// move the revealer
					if (!isRevealed)
					{
						x = xOrg;
						y = yOrg;
					}
					else
					{
						x = xOrg + xDelta;
						y = yOrg + yDelta;
					}
				}

				var actorToReveal:Actor = locationParent.actor(revealActor);
				if (actorToReveal != null)
				{
					actorToReveal.hidden = !isAllRevealed;
					glyph = isRevealed ? propertyGet('glyphRevealed', name) : propertyGetOrg('glyph', name);
				}
			}
		}

		var moveDeltasX:Array<Float> = propertiesGetAsFloat('moveDeltaX');
		var moveDeltasY:Array<Float> = propertiesGetAsFloat('moveDeltaY');
		if ((moveDeltasX != null) && (moveDeltasY != null))
		{
			var index:Int = propertyGetAsInt('moveIndex', -1);
			if (index == -1)
			{	// no value, so we're at the org
				x = xOrg;
				y = yOrg;
			}
			else if ((index < moveDeltasX.length) && (index < moveDeltasY.length))
			{
				x = xOrg + moveDeltasX[index];
				y = yOrg + moveDeltasY[index];
			}
		}

		if (propertyGetAsBool('toggle'))
		{
			var newName:String = (propertyGetAsBool('toggled')) ? 'glyphToggled' : propertyGetOrg('glyph', name);
			glyph = propertyExists(newName) ? propertyGet(newName) : propertyGetOrg('glyph', name);
		}
	}

	public function onTap(numTaps:Int = 1)
	{
		if (numTaps == 1)
		{	// see if we're a relocater
			var soundPlayed:Bool = false;

			var relocated:Bool = false;
			var relocaterDest:String = propertyGet('relocaterDest');
			if (relocaterDest != null)
			{
				var doRelocate:Bool = true;
				if (propertyGetAsBool('toggle'))
					doRelocate = propertyGetAsBool('toggled');	// if this is also a toggle, it must be toggled before the relocate will work

				if (doRelocate)
				{
					var locNew:Location = worldParent.location(relocaterDest);
					if ((locNew != null) && (locNew != locNew.worldParent.locationCurrent))
					{	// this is a real location and we're actually moving
						if (!soundPlayed)
							soundPlayed = soundPlayEvent('soundRelocate');

						locNew.makeCurrent(propertyGetAsFloat('relocaterDestTime', 0));
						relocated = true;
					}
				}
			}

			// see if we're a revealer
			var revealed:Bool = false;
			var revealTrigger:String = propertyGet('revealTrigger');
			if ((revealTrigger == null) || (propertyGetAsBool('revealTriggered')))
			{	// either there's no trigger or there is a trigger and it's been revealed already
				var revealActors:Array<String> = propertiesGet('revealActor');
				var revealLoop:Bool = propertyGetAsBool('revealLoop', true);
				var changed:Bool = false;
				if (revealActors != null)
				{
					for (revealActor in revealActors)
						if (!propertyGetAsBool(revealActor+'Revealed') || revealLoop)
						{	// the revealer is named and marked as revealed, but not the actors
							var nameNew:String = nameOrg;
							if (propertyFlipBool(revealActor+'Revealed'))
								nameNew += 'Revealed';
							name = nameNew;

							changed = true;
						}
				}

				var revealActorsSync:Array<String> = propertiesGet('revealActorSync');
				if (revealActorsSync != null)
				{
					if (revealLoop)
					{
						var stateFirst:Bool = propertyGetAsBool(revealActorsSync[0] + 'Revealed');
						for (revealActorIndex in 1...revealActorsSync.length)
						{
							var revealActor:String = revealActorsSync[revealActorIndex];

							if (propertyGetAsBool(revealActor + 'Revealed') != stateFirst)
							{
								var nameNew:String = nameOrg;
								if (propertyFlipBool(revealActor+'Revealed'))
									nameNew += 'Revealed';
								name = nameNew;
								changed = true;
								break;
							}

							if (!changed)
							{	// nothing changed, so change the first actor
								var nameNew:String = nameOrg;
								if (propertyFlipBool(revealActorsSync[0]+'Revealed'))
									nameNew += 'Revealed';
								name = nameNew;
								changed = true;
							}
						}
					}
					else
					{
						for (revealActor in revealActorsSync)
							if (!propertyGetAsBool(revealActor+'Revealed'))
							{
								propertyFlipBool(revealActor+'Revealed');
								changed = true;
								break;
							}
					}
				}

				if (changed)
				{
					revealed = true;
					if (!soundPlayed)
						soundPlayed = soundPlayEvent('soundReveal');
					worldParent.refresh(propertyGetAsFloat('revealTime', 0));
				}
			}

			var moved:Bool = false;
			var moveDeltasX:Array<String> = propertiesGet('moveDeltaX');
			var moveDeltasY:Array<String> = propertiesGet('moveDeltaY');
			if ((moveDeltasX != null) && (moveDeltasY != null))
			{
				var index:Int = propertyGetAsInt('moveIndex', -1) + 1;
				if (index >= Std.int(Math.min(moveDeltasX.length, moveDeltasY.length)))
					propertySet('moveIndex', null);	// kill it
				else
					propertySetAsFloat('moveIndex', index);

				refresh(propertyGetAsFloat('moveTime', 0));

				if (!soundPlayed)
					soundPlayed = soundPlayEvent('soundMove');

				moved = true;
			}

			var toggled:Bool = false;
			if (propertyGetAsBool('toggle'))
			{
				if ((propertyGet('toggleTrigger') == null) || (propertyGetAsBool('toggleTriggered')))
				{	// either there was no trigger, or there was a trigger and it's tripped
					if (!propertyGetAsBool('toggled') || propertyGetAsBool('toggleLoop', true))
					{
						var nameNew:String = nameOrg;
						if (propertyFlipBool('toggled'))
							nameNew += 'Toggled';

						name = nameNew;
						toggled = true;
						
						if (!soundPlayed)
							soundPlayed = soundPlayEvent('soundToggle');

						var revealTime:Float = propertyGetAsFloat('revealTime', 0);
						worldParent.refresh(revealTime);
						if (worldParent != World.worldMain)
							World.worldMain.refresh(revealTime);	// also refresh the main world in cast a toggle did something there
					}
				}
			}

			if (!relocated && !revealed && !moved && !toggled)
			{	// nothing happened, so see if this object has notes
				var suffix:String = '';
				if (propertyGetAsBool('toggled'))
					suffix = 'Toggled';

				var notes:Array<String> = propertiesGet('note'+suffix);

				if ((notes != null) && (notes.length != 0))
				{
					var worldNotes:World = World.world('notes');
					if (worldNotes != null)
					{
						var pod:NotePod = cast worldNotes.pod;
						pod.expand(true);
						pod.actor = this;

						if (!soundPlayed)
							soundPlayed = soundPlayEvent('soundNote'+suffix);

						for (i in 0...NotePod.maxNotes)	// light up the notes in this pod
							worldNotes.actor('note' + suffix + i).glyph = (i < notes.length) ? notes[i] : null;
					}
				}
			}
			else
			{	// hide the note pod if it's showing
				var worldNotes:World = World.world('notes');
				if (worldNotes != null)
					worldNotes.pod.expand(false);
			}
			
			if (!soundPlayed)
				soundPlayed = soundPlayEvent('soundTap');
		}
		else if (numTaps == 2)
		{
		}
	}

	public function onDrop(actorDropped:Actor):Bool
	{
		var retVal:Bool = false;
		var revealTrigger:String = propertyGet('revealTrigger');
		if ((revealTrigger != null) && (actorDropped.name == revealTrigger))
		{
			propertySetAsBool('revealTriggered', true);
			onTap();	// trigger the revealer
			if (!propertyGetAsBool('revealTriggerKeep'))	// the trigger now goes away because we didn't say we should keep it
				actorDropped.hidden = true;	// kill the trigger actor
		}

		var toggleTriggers:Array<String> = propertiesGet('toggleTrigger');
		if (toggleTriggers != null)
			for (toggleTrigger in toggleTriggers)
				if (actorDropped.name == toggleTrigger)
				{
					propertySetAsBool('toggleTriggered', true);
					onTap();	// trigger the toggle
					if (!propertyGetAsBool('toggleTriggerKeep'))	// the trigger now goes away because we didn't say we should keep it
						actorDropped.hidden = true;	// kill the trigger actor

					break;
				}

		return retVal;
	}

	public function refresh(_seconds:Float = 0, _secondsDelay:Float = 0, _ease:IEasing = null)
	{
		if (sprite != null)
		{
			onPlace();
			var xNew:Float = propertyGetAsFloat('x', sprite.x);
			var yNew:Float = propertyGetAsFloat('y', sprite.y);

			if (_seconds == 0)
			{	// no delay, so do everything now
				sprite.alpha = locationParent.alpha;
				sprite.visible = !hidden;	// assume not hidden
				sprite.x = xNew;
				sprite.y = yNew;
			}
			else
			{
				worldParent.acceptInput(false);

				var alphaNew = sprite.alpha;
				if (hidden)
					alphaNew = 0;	// disappear
				else
				{
					if (!sprite.visible)
					{	// make me visible
						sprite.visible = true;
						sprite.alpha = 0;
					}

					alphaNew = locationParent.alpha;
				}

				Actuate.tween
				(
					sprite,
					_seconds,
					{
						x:xNew,
						y:yNew,
						alpha:alphaNew
					},
					false
				)
				.ease((_ease == null) ? Linear.easeNone : _ease)
				.delay(_secondsDelay);

				worldParent.acceptInput(true, _seconds + _secondsDelay);	// wake up after this amount of time
			}
		}
	}

	public function new (_name:String, _parentLocation:Location, _properties:Map<String, String> = null, _x:Float = 0, _y:Float = 0)
	{
		super(_name, _parentLocation, _properties);

		propertiesOrg['x'] = Std.string(_x);	// save these
		propertiesOrg['y'] = Std.string(_y);

		propertiesOrg['parentLocationNameOrg'] = _parentLocation.nameOrg;
		propertiesOrg['parentWorldNameOrg'] = _parentLocation.worldParent.nameOrg;

		sprite = new Sprite();
		worldParent.sprite.addChildAtZ(sprite, 10);

		glyph = get_glyph();	// set the initial glyph state
	}
}

class PodControllerActor extends Actor
{	// an actor that lives in a pod. It has the ability to expand/retract the pod when clicked
	public override function onTap(numTaps:Int = 1)
	{
		super.onTap(numTaps);
		worldParent.pod.expand(!worldParent.pod.expanded(), Pod.tweenTimeDefault);
	}
}

class AccepterActor extends Actor
{	// an actor that accepts anything dropped on it
	public override function onDrop(actorDropped:Actor):Bool
	{
		var retVal:Bool = true;
		var actors:Array<Actor> = locationParent.actors;
			for (actor in actors)	// see if any actors are already on top of me
				if ((actor != this) && (!actor.hidden) && (actor.x == x) && (actor.y == y))
				{
					retVal = false;
					break;
				}

		if (retVal)
		{	// allow the drop to happen because there's not something already on me
			actorDropped.x = x;
			actorDropped.y = y;
		}
		return retVal;
	}
}

class DispatcherActor extends PodControllerActor
{	// an actor that hands off anything dragged on it to an empty accepter in the same location
	public override function onDrop(actorDropped:Actor):Bool
	{
		var retVal = false;
		for (actor in locationParent.actors)
			if (Type.enumEq(Type.typeof(actor), TClass(AccepterActor)))
			{	// this is an accepter. See if it has an actor on it
				var isAvailable:Bool = true;
				for (otherActor in locationParent.actors)
					if ((otherActor != actor) && (otherActor.x == actor.x) && (otherActor.y == actor.y))
					{	// this actor is not me but is in the same place as me, so don't put it here
						isAvailable = false;
						break;
					}

				if (isAvailable)
				{	// this slot is available and doesn't have anyone sitting on it, so let it be here
					actor.onDrop(actorDropped);	// here, hold this
					retVal = true;	// and this was a good drag
					break;	// and we're done
				}
			}

		return retVal;
	}
}

class SoundTogglerActor extends Actor
{
	public override function onPlace()
	{
		super.onPlace();
		Sounds.muted = (name != nameOrg);
	}
}

class MusicTogglerActor extends Actor
{
	public override function onPlace()
	{
		super.onPlace();
		Sounds.mutedTheme = (name != nameOrg);
	}
}

class URLActor extends Actor
{
	public override function onTap(numTaps:Int = 1)
	{
		super.onTap(numTaps);
		if (propertyExists('URL'))
			Lib.getURL(new URLRequest(propertyGet('URL')));
	}
}

class MapPinActor extends Actor
{
	public override function new (_name:String, _parentLocation:Location, _properties:Map<String, String> = null, _x:Float = 0, _y:Float = 0)
	{
		super(_name, _parentLocation, _properties, _x, _y);
		
		var info:String = World.progressInfo(Std.parseInt(_name));
		if (info != null)
		{	// show this as saved;
			propertySetAsBool('drawBackground', true);
			propertySetAsBool('item', true);
			glyph = '_podInfoPin';
		}
	}

	public override function onTap(numTaps:Int = 1)
	{
		if (glyph == '_podInfoPinAdd')
		{	// save a game here
			World.progressSave(Std.parseInt(nameOrg));
			propertySetAsBool('drawBackground', true);
			propertySetAsBool('item', true);
			glyph = '_podInfoPin';
		}
	}
}

class MapLoadActor extends Actor
{
	public override function onDrop(actorDropped:Actor):Bool
	{
		var retVal:Bool = false;

		if (actorDropped.glyph == '_podInfoPin')
		{	// load this saved game
			var slot:Int = Std.parseInt(actorDropped.nameOrg);
			World.endDrag(true);
			World.progressLoad(slot);
		}
		return retVal;
	}
}

class MapDeleteActor extends Actor
{
	public override function onDrop(actorDropped:Actor):Bool
	{
		var retVal:Bool = false;

		if (actorDropped.glyph == '_podInfoPin')
		{	// remove this saved game
			World.progressRemove(Std.parseInt(actorDropped.nameOrg));

			actorDropped.propertySetAsBool('drawBackground', false);
			actorDropped.propertySetAsBool('item', false);
			actorDropped.glyph = '_podInfoPinAdd';
			// put the thing back so it doesn't retract
			World.endDrag(true);
		}
		return retVal;
	}
}

class MapRevertActor extends Actor
{	// an actor that restarts everything
	public override function onTap(numTaps:Int = 1)
	{
		super.onTap(numTaps);
		for (world in World.worlds)
			world.revert();

		Game.thePlayingField.onStageResize(null);	// force everything to reposition itself
	}
}

class Location extends Entity
{	// a location is a collection of actors. It has no display, although it has an anchor point
	public var actors(default, null):Array<Actor>;

	public var worldParent(get, never):World;
	public inline function get_worldParent():World
		return cast entityParent;

	// x, y, width, height, and alpha look like member variables, but they're actually properties
	public var x(get, never):Float;
	public inline function get_x():Float
		return propertyGetAsFloat('x');

	public var y(get, never):Float;
	public inline function get_y():Float
		return propertyGetAsFloat('y');

	public var xOrg(get, never):Float;
	public inline function get_xOrg():Float
		return propertyGetOrgAsFloat('x');

	public var yOrg(get, never):Float;
	public inline function get_yOrg():Float
		return propertyGetOrgAsFloat('y');

	public var width(get, never):Float;
	public inline function get_width():Float
		return propertyGetAsFloat('width');

	public var height(get, never):Float;
	public inline function get_height():Float
		return propertyGetAsFloat('height');

	public var widthOrg(get, never):Float;
	public inline function get_widthOrg():Float
		return propertyGetOrgAsFloat('width');

	public var heightOrg(get, never):Float;
	public inline function get_heightOrg():Float
		return propertyGetOrgAsFloat('height');

	public var alpha(get, set):Float;
	public inline function get_alpha():Float	// note that alpha defaults to one
		return propertyGetAsFloat('alpha', 1);
	public inline function set_alpha(_f:Float):Float
		return propertySetAsFloat('alpha', _f);
	
	public var bounds(never, set):Rectangle;
	public inline function set_bounds(_bounds:Rectangle):Rectangle
	{	// first time in (constructor) set these as original
		if (propertyExists('x'))
			propertySetAsFloat('x', _bounds.x);
		else
			propertiesOrg['x'] = Std.string(_bounds.x);
			
		if (propertyExists('y'))
			propertySetAsFloat('y', _bounds.y);
		else
			propertiesOrg['y'] = Std.string(_bounds.y);

		if (propertyExists('width'))
			propertySetAsFloat('width', _bounds.width);
		else
			propertiesOrg['width'] = Std.string(_bounds.width);

		if (propertyExists('height'))
			propertySetAsFloat('height', _bounds.height)
		else
			propertiesOrg['height'] = Std.string(_bounds.height);
		
		return _bounds;
	}

	public function actor(_name:String, _world:String = null):Actor
	{
		var retVal:Actor = null;
		if (_world != null)
			retVal = worldParent.actor(_name, _world);	// find in the given world
		else if (_name.indexOf(Entity.strMarkerDereference) != -1)
			retVal = worldParent.actor(_name);	// this is a dot notation, so send it upstairs
		else
		{	// find in me
			for (actor in actors)
				if (actor.name == _name)
				{
					retVal = actor;
					break;
				}
		}

		return retVal;
	}

	public function takeActor(actor:Actor, logFrom:Bool = true)
	{	// takes an actor from another location, both in the array and in presentation (i.e. the sprite)
		if ((actor != null) && (actors.indexOf(actor) == -1))
		{	// good actor and not already here, so move it over
			if (logFrom && (actor.worldParent != null))
			{	// save the original world in case I wanna persist this
				var worldOrgName:String = actor.worldParent.nameOrg;
				if (!actor.propertyExists('worldOrg'))
					actor.propertySet('worldOrg', worldOrgName);
			}

			if (actor.locationParent != null)
				actor.locationParent.actors.remove(actor);	// remove me from the old location

			actors.push(actor);
			actor.entityParent = this;	// and add me to the new one

			actor.worldParent.sprite.addChildAtZ(actor.sprite, ZManager.zGet(actor.sprite));	// put it in the location
			actor.refresh();
		}
	}

	public function makeCurrent(_seconds:Float = 0, _secondsDelay:Float = 0, _ease:IEasing = null):Location
	{
		if (worldParent != null)
		{
			var scaleXNew:Float = worldParent.boundsView.width / width;
			var scaleYNew:Float = worldParent.boundsView.height / height;
			var scaleNew:Float = Math.min(scaleXNew, scaleYNew);
			var xNew = -(x * scaleNew);
			var yNew = -(y * scaleNew);
			var secondsTotal:Float = _seconds + _secondsDelay;

			var colorFloorPrev:Int = World.spriteFloor.transform.colorTransform.color;
			// next color is either set in the location props or in the world props (or 0 if neither)
			var colorFloorNext:Int = propertyGetAsInt('colorBackground', worldParent.propertyGetAsInt('colorBackground', ColorPreset.black));

			if (secondsTotal == 0)
			{
				worldParent.sprite.scaleX = worldParent.sprite.scaleY = scaleNew;
				worldParent.sprite.x = xNew;
				worldParent.sprite.y = yNew;

				if (worldParent.locationCurrent != null)
					for (actor in worldParent.locationCurrent.actors)
						if (actor.sprite != null)
							actor.sprite.visible = false;
							
				if (colorFloorPrev != colorFloorNext)
					CZHelpers.tint(World.spriteFloor, colorFloorNext, 1);
			}
			else
			{
				worldParent.acceptInput(false);

				var easeToUse:IEasing = (_ease == null) ? Cubic.easeIn : _ease;

				Actuate.tween
				(
					worldParent.sprite,
					_seconds,
					{
						x:xNew,
						y:yNew,
						scaleX:scaleNew,
						scaleY:scaleNew
					},
					false
				)
				.ease(easeToUse)
				.delay(_secondsDelay);

				if (colorFloorPrev != colorFloorNext)
				{
					Actuate.transform 
					(
						World.spriteFloor,
						_seconds
					)
					.color(colorFloorNext)
					.ease(easeToUse)
					.delay(_secondsDelay);
				}

				if (worldParent.locationCurrent != null)
					for (actor in worldParent.locationCurrent.actors)
						if (actor.sprite != null)
						{
							Actuate.tween
							(
								actor.sprite,
								_seconds,
								{
									alpha:0
								},
								false
							)
							.ease(easeToUse)
							.delay(_secondsDelay);
						}

				worldParent.acceptInput(true, secondsTotal);	// wake up after this amount of time
			}

			worldParent.locationCurrent = this;
			if (!worldParent.propertiesOrg.exists('locationOrg'))
				worldParent.propertiesOrg['locationOrg'] = nameOrg;

			Actuate.timer(secondsTotal).onComplete
			(
				function(_l:Location)
				{
					_l.soundPlayThemeEvent('soundThemeEnter');
					_l.soundPlayEvent('soundEnter');
				},
				[ worldParent.locationCurrent ]
			);

			worldParent.refresh();

			for (actor in worldParent.locationCurrent.actors)
				if (actor.sprite != null)
					actor.sprite.visible =  !actor.hidden;

			Pod.retractAll(Pod.tweenTimeDefault);
		}

		return worldParent.locationCurrent;
	}

	public function setFromTmxGroup(_group:TmxObjectGroup)
	{
		if (_group.properties != null)
			for (key in _group.properties.keys())
				propertiesOrg[key] = _group.properties[key];

		// tiled maps don't put actor registration points in the center, so here's the offsets to move 'em
		var xOffset:Float = worldParent.widthTile / 2;
		var yOffset:Float = -worldParent.heightTile / 2;

		for (ob in _group.objects)
		{	// first find the extremes
			var actor:Actor = null;

			if ((actor == null) &&  (ob.type != null))
			{	// there's a class specified in the object, so create an instance of that
				var type = Type.resolveClass(ob.type);
				if (type != null)	// be sure the class is real before instantiating it
					actor = Type.createInstance(type, [ob.name, this, ob.properties, ob.x + xOffset, ob.y + yOffset]);
			}

			var actorMapClass = Type.resolveClass('ActorMap');
			if ((actor == null) && (actorMapClass != null) && (Type.getClassFields(actorMapClass).indexOf('map') != -1))
			{	// there's a valid static actormap class with a map member, so see if the object exists in the actormap
				var map:Map<String, Class<Actor>> = Reflect.field(actorMapClass, 'map');
				if (map.exists(ob.name))	// this object has a behavior defined in ActorMap
					actor = Type.createInstance(map[ob.name], [ob.name, this, ob.properties, ob.x + xOffset, ob.y + yOffset]);
			}

			if ((ob.type != null) && (ob.type.toLowerCase() == 'bounds'))	// bounds objects don't actually get added to the list
				bounds = new Rectangle(ob.x, ob.y, ob.width, ob.height);	// don't add this object. This is just to calculate bounds
			else if (actor == null)	// none of these happened, so just create a standard object
				actor = new Actor(ob.name, this, ob.properties, ob.x + xOffset, ob.y + yOffset);

			if (actor != null)
				actors.push(actor);
		}
	}

	public override function revert()
	{	// returns this location to its original state
		var index:Int = actors.length;
		while (--index >= 0)
		{	// loop backwards through the array so we can remove elements
			var actor:Actor = actors[index];
			var locParentNameOrg:String = actor.propertyGetOrg('parentLocationNameOrg');
			var locWorldNameOrg:String = actor.propertyGetOrg('parentWorldNameOrg');
			if ((locParentNameOrg != nameOrg) || (locWorldNameOrg != worldParent.nameOrg))
				worldParent.location(locParentNameOrg, locWorldNameOrg).takeActor(actor, false);	// this had a different parent, so move it back
			actor.revert();
		}

		super.revert();
	}

	public function new (_name:String, _parentWorld:World, _properties:Map<String, String> = null, _group:TmxObjectGroup = null, _bounds:Rectangle = null)
	{
		actors = [];

		super(_name, _parentWorld, _properties);

		if (_bounds != null)
			bounds = _bounds;

		if (_group != null)
			setFromTmxGroup(_group);
	}
}

class World extends Entity
{
	private var locations(default, null):Array<Location>;
	public var locationCurrent:Location;

	public static var worlds(default, null):Array<World> = [];
	static public var worldMain(default, null):World;	// handy pointer to the primary world

	public function onInit() null;		// override this to respond to the world being initialized
	public function onChanged() null;	// override this to respond to changes in the world
	public var persist(default, null):Bool;	// is this a persistent world

	public var sprite:Sprite;	// the container of everything
	static public var spriteFloor:Sprite = null;	// the floor bitmap below everything (but not parent of it)
	static public var spriteCeiling:Sprite = null;
	public static var actorBeingDragged(default, null):Actor = null;

	public var widthTile(default, null):Float;
	public var heightTile(default, null):Float;

	public var boundsView(default, null):Rectangle;

	private var acceptInputCount:Int;	// am I broadcasting input events or am I animating?

	public var touchDist(default, null):Float;	// distance from something for it to be considered a valid touch

	public var pod(get, never):Pod;	// cast the world's sprite to a pod if you can
	public function get_pod():Pod	// note that this is a static cast, so know what you're doing
		return cast sprite;

	public function acceptInput(_a:Bool=true, seconds:Float=0)
	{
		if (seconds == 0)
		{
			if (_a)
				acceptInputCount = Std.int(Math.max(acceptInputCount - 1, 0));	// don't go below zero
			else
				++acceptInputCount;
		}
		else
		{	// make it increment or decrement in the future
			Actuate.timer(seconds).onComplete
			(
				function(_w:World, _a:Bool)
				{
					_w.acceptInput(_a);
				},
				[ this, _a ]
			);
		}
	}

	public function onMouseDown(ptStage:Point):Bool
	{	// return true if the event was accepted
		var retVal = false;
		if (acceptInputCount <= 0)
		{
			retVal = (sprite.visible && sprite.hitTestPoint(ptStage.x, ptStage.y));

			var distClosest:Float = Math.NaN;
			var actorClosest:Actor = null;
			var ptClosest:Point = null;
			if (locationCurrent != null)
				for (actor in locationCurrent.actors)
				{
					var ptGlobalActor:Point = actor.sprite.parent.localToGlobal(new Point(actor.sprite.x, actor.sprite.y));
					var draggable:Bool = true;
					if ((actor.sprite == null) || !actor.sprite.visible || (actor.sprite.alpha == 0))
						draggable = false;	// not a valid or visible sprite
					else if (!actor.propertyGetAsBool('item') && !actor.propertyGetAsBool('draggable'))
						draggable = false;	// not marked as a draggable item
					else if (!actor.isFullyRevealed())	// if this is a revealer and it hasn't revealed everything yet, then it's not draggable
						draggable = false;

					if (draggable)
					{
						var dist:Float = Point.distance(ptStage, ptGlobalActor);
						if ((actorClosest == null) || (dist < distClosest))
						{
							distClosest = dist;
							actorClosest = actor;
							ptClosest = ptGlobalActor;
						}
					}
				}

			if ((actorClosest != null) && (distClosest < (touchDist * sprite.scaleX)))
			{	// put it on the drag surface
				actorBeingDragged = actorClosest;
				World.spriteCeiling.scaleX = sprite.scaleX;
				World.spriteCeiling.scaleY = sprite.scaleY;	// make the drag surface the same scale as the thing we're dragging from, so it doesn't change size
				var pSurface = World.spriteCeiling.globalToLocal(ptClosest);
				World.spriteCeiling.addChildAtZ(actorBeingDragged.sprite, ZManager.zGet(actorBeingDragged.sprite));
				actorBeingDragged.sprite.x = pSurface.x;
				actorBeingDragged.sprite.y = pSurface.y;
				actorBeingDragged.sprite.startDrag();
				actorBeingDragged.soundPlayEvent('soundItemDrag');
			}
		}
		return retVal;
	}

	public function onMouseUp(ptStage:Point, numTaps:Int = 1):Bool
	{	// return true if the event was accepted
		var retVal:Bool = false;
		if (acceptInputCount <= 0)
		{
			retVal = (sprite.visible && sprite.hitTestPoint(ptStage.x, ptStage.y));

			var p:Point = sprite.globalToLocal(ptStage);
			if (actorBeingDragged == null)
			{	// this was a standard tap
				var distClosest:Float = Math.NaN;
				var actorClosest:Actor = null;
				if (locationCurrent != null)
					for (actor in locationCurrent.actors)
					{
						if ((actor.sprite != null) && actor.sprite.visible && (actor.sprite.alpha > 0) && (actor != actorBeingDragged))
						{
							var dist:Float = Point.distance(p, new Point(actor.sprite.x, actor.sprite.y));
							if ((actorClosest == null) || (dist < distClosest))
							{
								distClosest = dist;
								actorClosest = actor;
							}
						}
					}

				if ((actorClosest != null) && (distClosest < (touchDist * actorClosest.sprite.scaleX)))
					actorClosest.onTap(numTaps);
			}
			else
			{
				actorBeingDragged.sprite.stopDrag();

				// see if we're dropping into a valid place in this world
				var distClosest:Float = Math.NaN;
				var actorsClosest:Array<Actor> = null;
				if (locationCurrent != null)
					for (actor in locationCurrent.actors)
					{
						if ((actor.sprite != null) && actor.sprite.visible && (actor.sprite.alpha > 0) && (actor != actorBeingDragged) && ((actor.x != actorBeingDragged.x) || (actor.y != actorBeingDragged.y)))
						{
							var dist:Float = Point.distance(p, new Point(actor.sprite.x, actor.sprite.y));
							if ((actorsClosest == null) || (dist < distClosest))
							{
								distClosest = dist;
								actorsClosest = [ actor ];
							}
							else if (dist == distClosest)	// there are >1 actors in the same spot, so add the next one
								actorsClosest.push(actor);
						}
					}

				if ((actorsClosest != null) && (distClosest <= (touchDist * sprite.scaleX)))
				{
					for (actorClosest in actorsClosest)
						if (actorClosest.onDrop(actorBeingDragged))
						{	// this was a good drop
							locationCurrent.takeActor(actorBeingDragged);
							actorBeingDragged.soundPlayEvent('soundItemDrop');
							actorBeingDragged = null;	// drag is accepted, so we're done
							break;	// and we're done
						}
				}
				else
				{	// dropped in the same place, so just register this as a tap
					var ptGlobal:Point = World.spriteCeiling.localToGlobal(new Point(actorBeingDragged.sprite.x, actorBeingDragged.sprite.y));
					var spriteOwner:Sprite = actorBeingDragged.worldParent.sprite;
					var ptLocal = spriteOwner.globalToLocal(ptGlobal);
					distClosest = Point.distance(ptLocal, new Point(actorBeingDragged.x, actorBeingDragged.y));

					if (distClosest <= touchDist * sprite.scaleX)
					{
						var actorToTap:Actor = actorBeingDragged;
						endDrag(true);
						actorToTap.onTap();	// dropped very close to the org, so register it as a tap
					}
				}
			}
		}
		return retVal;
	}

	public static function endDrag(immediate:Bool = false)
	{	// put everything back
		var dist:Float = 0;
		if (actorBeingDragged != null)
		{
			actorBeingDragged.sprite.stopDrag();

			var ptGlobal:Point = World.spriteCeiling.localToGlobal(new Point(actorBeingDragged.sprite.x, actorBeingDragged.sprite.y));
			var spriteOwner:Sprite = actorBeingDragged.worldParent.sprite;
			var ptLocal = spriteOwner.globalToLocal(ptGlobal);
			spriteOwner.addChildAtZ(actorBeingDragged.sprite, ZManager.zGet(actorBeingDragged.sprite));	// put it back
			actorBeingDragged.sprite.x = ptLocal.x;
			actorBeingDragged.sprite.y = ptLocal.y;

			if (!immediate && !actorBeingDragged.hidden)
				dist = Point.distance(ptLocal, new Point(actorBeingDragged.x, actorBeingDragged.y));
			actorBeingDragged.refresh(dist / 550);	// slide it back at a constant speed

			actorBeingDragged = null;	// and we are done
		}
		return dist;
	}

	public function onMouseMove(ptStage:Point)
	{	// note that only drag events come here. mouse-up moves are filtered out
		if (acceptInputCount <= 0)
		{
		}
	}

	public function refresh(seconds:Float = 0, delay:Float = 0)
	{	// note, refreshing the world is done in two passes in case placing changes stuff that has gotta get refreshed
		for (actor in locationCurrent.actors)
			actor.onPlace();

		for (actor in locationCurrent.actors)
			actor.refresh(seconds, delay);

		onChanged();
	}

	public static function refreshAll(seconds:Float = 0, delay:Float = 0)
	{
		for (world in worlds)
			world.refresh(seconds, delay);
	}

	public override function revert()
	{	// returns this world to its original state
		for (location in locations)
			location.revert();
		super.revert();	// revert the world

		locationCurrent = null;

		// put everything else back
		for (location in locations)	// place everything in the whole danged world so we can cache things up
		{
			if (location.propertyGetAsBool('locationStart') || (locationCurrent == null))
				locationCurrent = location;

			for (actor in location.actors)
				actor.onPlace();
		}

		locationCurrent.makeCurrent();	// finally go back to our default location
	}

	public inline function addLocation(_location:Location):Location
	{
		locations.push(_location);
		return _location;
	}

	public function location(_name:String, _worldName:String = null):Location
	{	// get a location from its name world.location notation is okay
		var retVal:Location = null;

		if (_worldName == '*')
		{
			for (world in worlds)
			{
				retVal = world.location(_name);
				if (retVal != null)
					break;
			}
		}
		if (_worldName != null)	// find in a specific world
		{
			var world:World = world(_worldName);
			if (world != null)
				retVal = world.location(_name);
		}
		else if (_name.indexOf(Entity.strMarkerDereference) != -1)
		{	// this is a dot notation, so. . .find in world.location
			var names:Array<String> = _name.split(Entity.strMarkerDereference);
			retVal = location(names[1], names[0]);
		}
		else if (_name != null)
		{
			for (location in locations)
				if (location.name == _name)
				{
					retVal = location;
					break;
				}
		}

		return retVal;
	}

	public static function world(_name:String):World
	{
		var retVal:World = null;

		for (w in worlds)
			if (w.name == _name)
			{
				retVal = w;
				break;
			}

		return retVal;
	}

	public function actor(_name:String, _worldName:String = null):Actor
	{	// sift through all the locations to find an actor
		var retVal:Actor = null;

		var worldsToSearch:Array<World> = [];

		if (_worldName == '*')
			worldsToSearch = worlds;	// search all worlds
		else if (_worldName != null)
		{	// we were given a world specifically
			for (world in worlds)
				if (world.name == _worldName)
				{
					worldsToSearch.push(world);
					break;
				}
		}
		else
		{	// no specific world, so look for a dereference operator
			if (_name.indexOf(Entity.strMarkerDereference) != -1)
			{
				var names:Array<String> = _name.split(Entity.strMarkerDereference);
				retVal = actor(names[1], names[0]);
			}
			else
				worldsToSearch.push(this);	// just search me
		}

		for (worldToSearch in worldsToSearch)
		{
			for (location in worldToSearch.locations)
			{
				retVal = location.actor(_name);
				if (retVal != null)
					break;
			}

			if (retVal != null)
				break;
		}

		return retVal;
	}

	public static function progressSave(_slot:Int):Bool
	{
		var retVal:Bool = false;

		var root:Xml = Xml.createElement('SaveGame');
		root.set('slot', Std.string(_slot));

		for (world in worlds)
			if (world.persist)
			{	// if this is marked as a persistent world, save it out
				var childWorld:Xml = world.toXML('w_');

				if (world.propertiesOrg['locationOrg'] != world.locationCurrent.nameOrg)
					childWorld.set('location', world.locationCurrent.nameOrg);	// at a different location, so save it

				for (location in world.locations)
				{
					var childLocation:Xml = location.toXML('l_');

					for (actor in location.actors)
					{
						var childActor:Xml = actor.toXML('a_');
						if (childActor.attributes().hasNext() || childActor.elements().hasNext())
							childLocation.addChild(childActor);
					}

					if (childLocation.attributes().hasNext() || childLocation.elements().hasNext())
						childWorld.addChild(childLocation);
				}
					
				if (childWorld.attributes().hasNext() || childWorld.elements().hasNext())
					root.addChild(childWorld);
			}

		var so:SharedObject = SharedObject.getLocal('save_' + _slot);
		if (so != null)
		{
			Reflect.setProperty(so.data, 'Xml', root.toString());
			Reflect.setProperty(so.data, 'timeStamp', Std.string(Date.now().getTime()));
			retVal = true;
		}

		return retVal;
	}

	public static function progressLoad(_slot:Int):Bool
	{
		var retVal:Bool = false;

		for (world in World.worlds)
			world.revert();

		var so:SharedObject = SharedObject.getLocal('save_' + _slot);
		if ((so != null) && Reflect.hasField(so.data, 'Xml'))
		{
			var fileXml:Xml = Xml.parse(Reflect.getProperty(so.data, 'Xml'));
			if (fileXml != null)
			{
				for (worldXml in fileXml.firstChild())	// skip over the main XML wrapper, as all the goodies are inside
				{
					var locationNew:String = null;
					var worldName:String = worldXml.nodeName.substr(2);
					var world:World = World.world(worldName);
					if (world != null)
					{
						for (attribute in worldXml.attributes())
						{	// set this world's attributes
							var value:String = worldXml.get(attribute);
							if (attribute == 'location')
								locationNew = value;
							else
								world.propertySet(attribute, value);
						}

						for (locationXml in worldXml)
						{
							var locationName:String = locationXml.nodeName.substr(2);
							var location:Location = world.location(locationName);
							if (location != null)
							{
								for (attribute in locationXml.attributes())
									location.propertySet(attribute, locationXml.get(attribute));

								for (actorXml in locationXml)
								{
									var actorName:String = actorXml.nodeName.substr(2);
									var actor:Actor = location.actor(actorName);
									
									if ((actor == null) && (actorXml.exists('worldOrg')))
									{	// this actor came from a different world, so move it here
										actor = location.actor(actorName, actorXml.get('worldOrg'));
										location.takeActor(actor);	// yep, there it is. Move it over
									}

									if (actor != null)
										for (attribute in actorXml.attributes())
											actor.propertySet(attribute, actorXml.get(attribute));
								}
							}
						}

						if ((locationNew != null) && (locationNew != world.propertiesOrg['locationOrg']))
							world.location(locationNew).makeCurrent();	// move to this location
					}
				}
			}

			retVal = true;
		}

		World.refreshAll();
		Game.thePlayingField.onStageResize(null);	// force everything to reposition itself

		return retVal;
	}

	public static function progressRemove(_slot:Int):Bool
	{
		var retVal:Bool = false;

		var so:SharedObject = SharedObject.getLocal('save_' + _slot);
		if (so != null)
		{
			so.clear();
			retVal = true;
		}

		return retVal;
	}

	public static function progressInfo(_slot:Int):String
	{
		var retVal:String = null;

		var so:SharedObject = SharedObject.getLocal('save_' + _slot);
		if ((so != null) && Reflect.hasField(so.data, 'timeStamp'))
		{
			var d:Date = Date.fromTime(Std.parseFloat(Reflect.getProperty(so.data, 'timeStamp')));
			retVal = DateTools.format(d, '%F\n%r');
		}

		return retVal;
	}

	public function setFromTmxMap(_map:TmxMap, _prefix:String = null)
	{	// first get any properties from this group and add them to the world
		widthTile = _map.tileWidth;
		heightTile = _map.tileHeight;
		sprite.scaleX = sprite.scaleY = 1;

		if (_map.properties != null)
			for (key in _map.properties.keys())
				propertiesOrg[key] = _map.properties[key];

		locationCurrent = null;
		for (layer in _map.layers)
			switch(layer)
			{
				case ObjectGroup(group):
					var groupName:String = null;
					if (_prefix != null)
					{	// only load layers with 'world.location' names
						var names:Array<String> = group.name.split(Entity.strMarkerDereference);
						if (names[0].toLowerCase() == _prefix.toLowerCase())
						{
							if (names.length == 1)
								groupName = 'default';
							else 
								groupName = names[1];
						}
					}
					else
						groupName = group.name;

					if (groupName != null)
					{
						var locNew:Location = new Location(groupName, this, null, group);
						addLocation(locNew);

						for (actor in locNew.actors)	// hide the actors, as we'll reveal them when we move into the first room
							if (actor.sprite != null)
								actor.sprite.visible = false;

						if (locNew.propertyGetAsBool('locationStart') || (locationCurrent == null))
							locationCurrent = locNew;
					}

				default:
					// skip other layer types
			}
	}

	public function new (_name:String, _sprite:Sprite, _bounds:Rectangle = null, _map:TmxMap = null, _widthTileDefault:Float = 52, _heightTileDefault:Float = 52)
	{
		locations = [];
		locationCurrent = null;
		widthTile = _widthTileDefault;
		heightTile = _heightTileDefault;
		actorBeingDragged = null;
		acceptInputCount = 0;
		sprite = _sprite;
		persist = true;	// worlds persist by default. Create a transient world if you don't want this

		if (_bounds != null)
			boundsView = new Rectangle(_bounds.x, _bounds.y, _bounds.width, _bounds.height);
		else
			boundsView = new Rectangle(0, 0, sprite.width, sprite.height);

		if (_name == null)
			worldMain = this;

		super(_name);

		if (_map != null)
		{
			setFromTmxMap(_map, _name);
			onInit();	// call oninit before the first refresh
			locationCurrent.makeCurrent();
		}
		else
			onInit();

		touchDist = Math.max(widthTile, heightTile) / 2;

		for (location in locations)	// place everything in the whole danged world so we can cache things up
			for (actor in location.actors)
				actor.onPlace();

		for (location in locations)
			for (actor in location.actors)
				actor.propertiesCommit();	// commit changes, for stuff like auto-hide so they don't persist

		worlds.push(this);	// add myself to the static world-list
	}
}

class TransientWorld extends World
{	// just a standard world with the persistent bit turned off
	public function new (_name:String, _sprite:Sprite, _bounds:Rectangle = null, _map:TmxMap = null, _widthTileDefault:Float = 52, _heightTileDefault:Float = 52)
	{
		super(_name, _sprite, _bounds, _map, _widthTileDefault, _heightTileDefault);
		persist = false;
	}
}