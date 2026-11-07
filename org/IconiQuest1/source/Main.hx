import openfl.*;
import openfl.display.*;
import openfl.events.*;
import openfl.net.*;
import openfl.geom.*;

import format.tmx.Reader;
import format.tmx.Data.TmxMap;

#if (free)
	import extension.admob.AdMob;
	import extension.admob.GravityMode;
#end

#if (GPG)
	import extension.gpg.GooglePlayGames;
#end

using ZManager;
using CZHelpers;

import Listener;
import TimeKeeper;
import TextureAtlasSparrow;
import TextureAtlasAngel;
import TextureAtlasRect;

import Entity;
import EntityCustom;

class Field extends PlayingField.SizingPlayingField
{
	private static var wasPaused:Bool = false;

	private var timeClickLast:Int;
	private var ptClickLast:Point;
	private static inline var timeDoubleClick:Int = 500;

	static private var mouseDown:Bool = false;

	override public function onInit()
	{
		super.onInit();

		TimeKeeper.createInstanceStatic();
		TimeKeeper.instance.paused = true;

		Random.createInstanceStatic();

		ptClickLast = new Point();
		mouseDown = false;
		timeClickLast = 0;

		addChildAtZ(World.spriteFloor = new Sprite(), 0);
		World.spriteFloor.graphics.beginFill(0);
		World.spriteFloor.graphics.drawRect(-Game.instance.boundsPlayingField.width, -Game.instance.boundsPlayingField.height, Game.instance.boundsPlayingField.width * 3, Game.instance.boundsPlayingField.height * 3);
		World.spriteFloor.graphics.endFill();

		// load the bitmaps
		GlyphCache.createInstanceStatic();
		GlyphCache.instance.atlasAngel = new TextureAtlasAngelCached(Xml.parse(Assets.getText('assets/text.fnt')), 'assets');

		// set a default size for the pods
		var podBoundsDefault:Rectangle = new Rectangle(0, 0, Game.instance.boundsPlayingField.width / 2, Game.instance.boundsPlayingField.height / 2);

		// create the inventory world
		var locInventory = new Location
		(
			'default',
			new World('inventory', new Pod(podBoundsDefault.width, podBoundsDefault.height, -1, 1)),
			null,
			null,
			podBoundsDefault
		);
		locInventory.actors.push(new DispatcherActor('_podInventory', locInventory, [ 'drawBackground' => 'false' ], podBoundsDefault.width - (Pod.amountOnScreen / 2), Pod.amountOnScreen / 2));	// the inventory icon
		locInventory.actors.push(new PodControllerActor('_podClose', locInventory, [ 'drawBackground' => 'false' ], Pod.amountOnScreen / 2, Pod.amountOnScreen / 2));
		var inventoryWidth:Int = 5;
		var inventoryHeight:Int = 3;
		var gutterX:Int = 18;
		var gutterY:Int = 12;
		for (i in 0...inventoryHeight)
			for (j in 0...inventoryWidth)
				locInventory.actors.push
				(
					new AccepterActor
					(
						'_podInventoryBlank',
						locInventory,
						[ 'drawBackground' => 'false' ],
						(podBoundsDefault.width - (Pod.amountOnScreen / 2)) - (j * (locInventory.worldParent.widthActorDefault + gutterX)) - gutterX/2,
						(Pod.amountOnScreen / 2) + ((i + 1) * (locInventory.worldParent.heightActorDefault + gutterY)) - gutterY/2
					)
				);
		locInventory.worldParent.addLocation(locInventory).makeCurrent();
		stage.addChildAtZ(locInventory.worldParent.sprite, 500);

		// create the info world
		var locInfo = new Location
		(
			'default',
			new TransientWorld('settings', new Pod(podBoundsDefault.width, podBoundsDefault.height, 1, 1, 0xD0B0B0, 0xD0B0B0)),
			null,
			null,
			podBoundsDefault
		);
		locInfo.actors.push(new PodControllerActor('_podInfo', locInfo, [ 'drawBackground' => 'false' ], Pod.amountOnScreen / 2, Pod.amountOnScreen / 2));
		locInfo.actors.push(new PodControllerActor('_podClose', locInfo, [ 'drawBackground' => 'false' ], podBoundsDefault.width - (Pod.amountOnScreen / 2), Pod.amountOnScreen / 2));
		locInfo.actors.push(new PodControllerActor('score', locInfo, [ 'drawBackground' => 'false' ], podBoundsDefault.width / 2, Pod.amountOnScreen / 2));

		var numCols:Int = 4;
		var numRows:Int = 3;
		var pinIndex:Int = 0;
		for (i in 0...numRows)
		{
			var y:Float = CZHelpers.centerItem(podBoundsDefault.height - Pod.amountOnScreen, i, numRows, locInfo.worldParent.heightActorDefault) + locInfo.worldParent.heightActorDefault / 2 + Pod.amountOnScreen;
			if (i == 0)
			{	// top row
				locInfo.actors.push(new MapLoadActor('_podInfoMap', locInfo, [ 'drawBackground' => 'false', 'note' => '_podInfoPin' ], podBoundsDefault.width / 3, y));
				locInfo.actors.push(new MapDeleteActor('_podInfoTrash', locInfo, [ 'drawBackground' => 'false', 'note' => '_podInfoPin' ], podBoundsDefault.width * 2 / 3, y));
			}
			else
				for (j in 0...numCols)
				{
					var x:Float = CZHelpers.centerItem(podBoundsDefault.width, j, numCols, locInfo.worldParent.widthActorDefault) + locInfo.worldParent.widthActorDefault / 2;
					locInfo.actors.push(new MapPinActor(Std.string(pinIndex) + 'Pin', locInfo, [ 'glyph' => '_podInfoPinAdd', 'drawBackground' => 'false' ], x, y));
					++pinIndex;
				}
		}

		locInfo.worldParent.addLocation(locInfo).makeCurrent();
		stage.addChildAtZ(locInfo.worldParent.sprite, 500);

		// make the pod thin for the top ones
		podBoundsDefault.height = Pod.amountOnScreen;

		// create the web world
		var locWeb = new Location
		(
			'default',
			new TransientWorld('about', new Pod(podBoundsDefault.width, podBoundsDefault.height, -1, -1, 0xB0D0B0, 0xB0D0B0)),
			null,
			null,
			podBoundsDefault
		);
		locWeb.actors.push(new PodControllerActor('_podWeb', locWeb, [ 'drawBackground' => 'false' ], podBoundsDefault.width - (Pod.amountOnScreen / 2), podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locWeb.actors.push(new PodControllerActor('_podClose', locWeb, [ 'drawBackground' => 'false' ], Pod.amountOnScreen / 2, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locWeb.actors.push(new URLActor('_podWebGoogle', locWeb, [ 'drawBackground' => 'false', 'URL' => 'http://plus.google.com' ], podBoundsDefault.width / 2 - Pod.amountOnScreen * 1.3, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locWeb.actors.push(new URLActor('_podWebFacebook', locWeb, [ 'drawBackground' => 'false', 'URL' => 'http://facebook.com' ], podBoundsDefault.width / 2 - Pod.amountOnScreen * 0.43, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locWeb.actors.push(new URLActor('_podWebTwitter', locWeb, [ 'drawBackground' => 'false', 'URL' => 'http://twitter.com' ], podBoundsDefault.width / 2 + Pod.amountOnScreen * 0.43, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locWeb.actors.push(new URLActor('_podWebTheCodeZone', locWeb, [ 'drawBackground' => 'false', 'URL' => 'http://thecodezone.com' ], podBoundsDefault.width / 2 + Pod.amountOnScreen * 1.3, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locWeb.worldParent.addLocation(locWeb).makeCurrent();
		stage.addChildAtZ(locWeb.worldParent.sprite, 500);

		// create the settings world
		var locSettings = new Location
		(
			'default',
			new World('sound', new Pod(podBoundsDefault.width, podBoundsDefault.height, 1, -1, 0xB0B0D0, 0xB0B0D0)),
			null,
			null,
			podBoundsDefault
		);
		locSettings.actors.push(new PodControllerActor('_podSettings', locSettings, [ 'drawBackground' => 'false' ], Pod.amountOnScreen / 2, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locSettings.actors.push(new PodControllerActor('_podClose', locSettings, [ 'drawBackground' => 'false' ], podBoundsDefault.width - Pod.amountOnScreen / 2, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locSettings.actors.push(new SoundTogglerActor('_podSettingsSound', locSettings, [ 'drawBackground' => 'false', 'toggle' => 'true' ], (podBoundsDefault.width - Pod.amountOnScreen * 2) * 1 / 6 + Pod.amountOnScreen, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locSettings.actors.push(new MusicTogglerActor('_podSettingsMusic', locSettings, [ 'drawBackground' => 'false', 'toggle' => 'true' ],  (podBoundsDefault.width - Pod.amountOnScreen * 2) * 3 / 6 + Pod.amountOnScreen, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locSettings.actors.push(new MapRevertActor('_podSettingsRestart', locSettings, [ 'drawBackground' => 'false' ],  (podBoundsDefault.width - Pod.amountOnScreen * 2) * 5 / 6 + Pod.amountOnScreen, podBoundsDefault.height - (Pod.amountOnScreen / 2)));
		locSettings.worldParent.addLocation(locSettings).makeCurrent();
		stage.addChildAtZ(locSettings.worldParent.sprite, 500);

		// create the notes world
		var locNotes = new Location
		(
			'default',
			new TransientWorld('notes', new NotePod(podBoundsDefault.width, podBoundsDefault.height, 0, 0), null, null, 64, 64),
			null,
			null,
			podBoundsDefault
		);
		for (i in 0...NotePod.maxNotes)
			locNotes.actors.push(new Actor('note' + i, locNotes, [ 'drawBackground' => 'false', 'glyph' => 'blank' ], locNotes.worldParent.widthActorDefault / 2 + (locNotes.worldParent.widthActorDefault * i), locNotes.worldParent.heightActorDefault / 2));
		locNotes.worldParent.addLocation(locNotes).makeCurrent();
		stage.addChildAtZ(locNotes.worldParent.sprite, 501);

		// finally create the main world and the ceiling
		addChildAtZ(new MainWorld(null, new Sprite(), Game.instance.boundsPlayingField, new Reader(Xml.parse(Assets.getText('assets/mapMain.tmx'))).read()).sprite, 10);
		stage.addChildAtZ(World.spriteCeiling = new Sprite(), 1000);

		Listener.add
		(
			stage,
			MouseEvent.MOUSE_DOWN,
			function(evt:MouseEvent)
			{
				if (!TimeKeeper.instance.paused && !mouseDown)
				{
					var ptMouse:Point = new Point(evt.stageX, evt.stageY);

					trace('mousedown'+ptMouse);

					for (w in World.worlds)
						if (w.onMouseDown(ptMouse))
							break;	// something accepted the event

					mouseDown = true;
				}
			}
		);

		Listener.add
		(
			stage,
			MouseEvent.MOUSE_UP,
			function(evt:MouseEvent)
			{
				if (!TimeKeeper.instance.paused && mouseDown)
				{	// check for double tap
					var timeCurrent:Int = Lib.getTimer();
					var ptMouse:Point = new Point(evt.stageX, evt.stageY);

					if ((timeClickLast != 0) && (timeCurrent - timeClickLast <= timeDoubleClick))
					{
						if (World.actorBeingDragged != null)
							World.endDrag(true);	// no drag actually happened, so make sure drag is done

						trace('mouseup double');
						for (w in World.worlds)
							if (w.onMouseUp(ptMouse, 2))
								break;	// something absorbed the event

						timeClickLast = 0;	// and we're done
					}
					else
					{
						timeClickLast = timeCurrent;
						ptClickLast = ptMouse;

						var actorBeingDraggedOrg:Actor = World.actorBeingDragged;

						trace('mouseup');
						for (w in World.worlds)
							if (w.onMouseUp(ptMouse) || (actorBeingDraggedOrg != World.actorBeingDragged))
								break;	// something accepted the or the mouseup, so we don't need to look further

						if (actorBeingDraggedOrg == World.actorBeingDragged)
							World.endDrag(true);	// no drag actually happened, so make sure drag is done
					}
				}

				mouseDown = false;
			}
		);

		Listener.add
		(
			stage,
			MouseEvent.MOUSE_MOVE,
			function(evt:MouseEvent)
			{
				if (!TimeKeeper.instance.paused && mouseDown)
				{
					var ptMouse:Point = new Point(evt.stageX, evt.stageY);
					for (w in World.worlds)
						w.onMouseMove(ptMouse);
				}
			}
		);

		#if (free)
			AdMob.showBanner();
		#end

		TimeKeeper.instance.paused = false;	// and start things moving
	}

	override public function onTick(currentTime:Int, deltaTime:Int, numTicks:Int)
	{
		super.onTick(currentTime, deltaTime, numTicks);

		if (!TimeKeeper.instance.paused)
		{
		}

		if (wasPaused != TimeKeeper.instance.paused)
			wasPaused = TimeKeeper.instance.paused;
	}
	
	override public function onStageResize(e:Event)
	{
		for (p in Pod.pods)
			p.reposition();	// reposition the pods
	}
}

class Main extends Game
{
	public function new()
	{
		super('Adventure', 720, 540);

		#if (free)
	        AdMob.initAndroid('ca-app-pub-9964264073544332/1413880699', '', GravityMode.BOTTOM);
			AdMob.initIOS('ca-app-pub-9964264073544332/4367347090', '', GravityMode.BOTTOM);
		#end

		#if (GPG)
			GooglePlayGames.init(Game.instance.stage, false);
		#end

		run(new Field());
	}
}